#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include "ETWTI.h"
#include "MemHandler.h"
#include <map>

// I can hear the OSR replies now... 
#define ProviderEnableInfo_OFFSET 0x60
#define GuidEntry_OFFSET 0x20

//lkd > u
//nt!KeInsertQueueApc + 0x12:
//fffff806`8f880392 4155            push    r13
//fffff806`8f880394 4156            push    r14
//fffff806`8f880396 4157            push    r15
//fffff806`8f880398 4883ec70        sub     rsp, 70h
//fffff806`8f88039c 4c8b15b5dfc700  mov     r10, qword ptr[nt!EtwThreatIntProvRegHandle(fffff806`904fe358)]
//	fffff806`8f8803a3 458be9          mov     r13d, r9d
//	fffff806`8f8803a6 488be9          mov     rbp, rcx
//	fffff806`8f8803a9 4d85d2          test    r10, r10

const uint8_t patternEtwThreatIntProvRegHandle[] = { 0x45, 0x8b, 0xe9, 0x48, 0x8b, 0xe9 };

class ETWTI
{
public:
	ETWTI(MemHandler* objMemHandler);
	~ETWTI();
	PVOID lpNtosBase = { 0 };
	PVOID lpnetioBase = { 0 };
	std::map<DWORD64, std::pair<DWORD64, DWORD64>> patchCallbackMap;
	std::map<DWORD64, std::pair<DWORD64, DWORD64>> patchLinksMap;
	BOOL EnumerateETW(BOOLEAN REMOVE = false, wchar_t* DriverName = NULL);

private:
	ULONG ulNumFrames;
	PVOID ResolveDriverBase(const wchar_t* strDriverName);
	MemHandler* objMemHandler;
};
