#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include "net.h"
#include "MemHandler.h"
#include <map>

#define UNISTR_OFFSET_LEN 0
#define UNISTR_OFFSET_BUF 8
//lkd > u netio!FeInitCalloutTable L9
//NETIO!FeInitCalloutTable:
//	fffff802`3e575e54 4053            push    rbx
//	fffff802`3e575e56 4883ec20        sub     rsp, 20h
//	fffff802`3e575e5a 488b05df970500  mov     rax, qword ptr[NETIO!gWfpGlobal(fffff802`3e5cf640)]
//	fffff802`3e575e61 0f57c0          xorps   xmm0, xmm0
//	fffff802`3e575e64 ba57667043      mov     edx, 43706657h
//	fffff802`3e575e69 b900800100      mov     ecx, 18000h
//	fffff802`3e575e6e 0f118098010000  movups  xmmword ptr[rax + 198h], xmm0
//	fffff802`3e575e75 4c8b05c4970500  mov     r8, qword ptr[NETIO!gWfpGlobal(fffff802`3e5cf640)] ; search for this
//	fffff802`3e575e7c 4981c0a0010000  add     r8, 1A0h ; as well search for this
const uint8_t patterngWfpGlobal[] = { 0x4C, 0x8B, 0x05, 0x49, 0x81, 0xC0 };

// Search for the structure Size
// Search First for Call InitDefaultCallout
// fffff806`3bb15ebc e81f000000      call    NETIO!InitDefaultCallout(fffff806`3bb15ee0)
// fffff806`3bb15ec1 488bd8          mov     rbx, rax
// fffff806`3bb15ec4 4885db          test    rbx, rbx
const uint8_t patterngInitDefaultCallout[] = { 0x48, 0x8B, 0xd8, 0x48, 0x85, 0xdb };

// Search for the structure size inside the function
// NETIO!InitDefaultCallout:
// fffff802`4dde5ee0 4053            push    rbx
// fffff802`4dde5ee2 4883ec20        sub     rsp, 20h
// fffff802`4dde5ee6 4c8d056b9f0500  lea     r8, [NETIO!gFeCallout(fffff802`4de3fe58)]
// fffff802`4dde5eed ba57667043      mov     edx, 43706657h
// fffff802`4dde5ef2 b960000000      mov     ecx, 60h
const uint8_t patterngCalloutStructureSize[] = { 0xb9 };

//lkd > u
//NETIO!InitDefaultCallout + 0x22:
//fffff801`63968166 7573            jne     NETIO!InitDefaultCallout + 0x97 (fffff801`639681db)
//fffff801`63968168 488b0de1860300  mov     rcx, qword ptr[NETIO!gFeCallout(fffff801`639a0850)]
//fffff801`6396816f 448d4060        lea     r8d, [rax + 60h]
//fffff801`63968173 33d2 xor edx, edx
//fffff801`63968175 e8c6cb0100      call    NETIO!_memset_spec_ermsb(fffff801`63984d40)
//fffff801`6396817a 488b0dcf860300  mov     rcx, qword ptr[NETIO!gFeCallout(fffff801`639a0850)]
//fffff801`63968181 488d0528ebfdff  lea     rax, [NETIO!FeDefaultClassifyCallback(fffff801`63946cb0)]
//fffff801`63968188 c70104000000    mov     dword ptr[rcx], 4

const uint8_t patterngFeDefaultClassifyCallback[] = { 0x48, 0x8d, 0x05, 0xc7, 0x01 };

class NetworkManager
{
public:
	NetworkManager(MemHandler* objMemHandler);
	~NetworkManager();
	PVOID lpNtosBase = { 0 };
	PVOID lpnetioBase = { 0 };
	std::map<DWORD64, std::pair<DWORD64, DWORD64>> patchCallbackMap;
	std::map<DWORD64, std::pair<DWORD64, DWORD64>> patchLinksMap;
	BOOL Restore();
	TCHAR* FindDriver(DWORD64 address);
	BOOL EnumerateNetworkFilters(BOOLEAN REMOVE = false, wchar_t* DriverName = NULL, DWORD64 ADDRESS = NULL);
	wchar_t* ExtractDriverName(TCHAR* driverOutput);

private:
	ULONG ulNumFrames;
	PVOID ResolveDriverBase(const wchar_t* strDriverName);
	MemHandler* objMemHandler;
};
