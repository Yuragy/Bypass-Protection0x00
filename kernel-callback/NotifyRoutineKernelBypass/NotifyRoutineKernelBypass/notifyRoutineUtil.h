// CallbackRemover.h
#pragma once

#include <Windows.h>
#include <aclapi.h>
#include <Psapi.h>
#include <cstdio>
#include <iostream>
#include <tchar.h>
#include <map>
#include "MemHandler.h"
#include <tlhelp32.h>

#define PRINT_ERROR_AUTO(func) (wprintf(L"ERROR " TEXT(__FUNCTION__) L" ; " func L" (0x%08x)\n", GetLastError()))

// Structure Definitions
struct Offsets {
    DWORD64 process;
    DWORD64 image;
    DWORD64 thread;
    DWORD64 registry;
};
inline Offsets getVersionOffsets() {
    wchar_t value[255] = { 0x00 };
    DWORD BufferSize = sizeof(value);

    if (RegGetValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ReleaseId", RRF_RT_REG_SZ, NULL, &value, &BufferSize) != ERROR_SUCCESS) {
        wprintf(L"[!] Failed to retrieve Windows version\n");
        return { 0, 0, 0, 0 };
    }

    wprintf(L"[+] Windows Version %s Found\n", value);
    int winVer = _wtoi(value);

    switch (winVer) {
    case 1909:
        return { 0x8b48cd0349c03345, 0xe8d78b48d90c8d48, 0xe8cd8b48f92c8d48, 0x4024448948f88b48 };
    case 2004:
        return { 0x8b48cd0349c03345, 0xe8d78b48d90c8d48, 0xe8cd8b48f92c8d48, 0x4024448948f88b48 };
    case 2009:
        return { 0x7340fe8341f63345, 0x8d48d68b48c03345, 0x48d90c8d48c03345, 0x4024448948f88b48 };
    default:
        wprintf(L"[!] Version Offsets Not Found!\n");
        return { 0, 0, 0, 0 };
    }
}

class notifyRoutine
{
public:
	notifyRoutine(MemHandler* objMemHandler);
	~notifyRoutine();
	PVOID lpNtosBase = { 0 };
	DWORD64 GetFunctionAddress(LPCSTR function);
	std::map<DWORD64, std::pair<DWORD64, DWORD64>> patchCallbackMap;
	std::map<DWORD64, std::pair<DWORD64, DWORD64>> patchLinksMap;
	BOOL Restore();
	DWORD64 PatternSearch(DWORD64 start, DWORD64 end, DWORD64 pattern);
	void findregistrycallbackroutines(DWORD64 remove);
	void unlinkregistrycallbackroutines(DWORD64 remove);
	void findimgcallbackroutine(DWORD64 remove);
	void findthreadcallbackroutine(DWORD64 remove);
	void findprocesscallbackroutine(DWORD64 remove);
	void findprocesscallbackroutinestealth(DWORD64 remove);
	TCHAR* FindDriver(DWORD64 address);
private:
	ULONG ulNumFrames;
	PVOID ResolveDriverBase(const wchar_t* strDriverName);
	MemHandler* objMemHandler;
};