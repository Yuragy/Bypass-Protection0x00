#define CUSTOM_ENTRY
#include "resource.h"
#include "r77def.h"
#include "r77win.h"
#include "r77config.h"
#include "r77process.h"

int main()
{
	EnabledDebugPrivilege();

	// Delete the stager executable and the rootkit DLL's.
	HKEY key;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE", 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &key) == ERROR_SUCCESS)
	{
		RegDeleteValueW(key, HIDE_PREFIX L"stager");
		RegDeleteValueW(key, HIDE_PREFIX L"dll32");
		RegDeleteValueW(key, HIDE_PREFIX L"dll64");
	}

	// Delete the Windows Service that starts the r77 service.
	DeleteWindowsService(R77_SERVICE_NAME);

	if (Is64BitOperatingSystem())
	{
		// On 64-bit Windows, Uninstall64.exe is extracted into the temp directory, executed and deleted afterwards.

		LPBYTE uninstall64;
		DWORD uninstall64Size;
		if (GetResource(IDR_UNINSTALL64, "EXE", &uninstall64, &uninstall64Size))
		{
			WCHAR uninstall64Path[MAX_PATH + 1];
			if (CreateTempFile(uninstall64, uninstall64Size, L"exe", uninstall64Path))
			{
				ExecuteFile(uninstall64Path, TRUE);
			}
		}
	}
	else
	{
		// Detach r77 service from its host process.
		DetachR77Service();

		// Detach all injected processes.
		DetachAllInjectedProcesses();
	}

	// Delete HKEY_LOCAL_MACHINE\SOFTWARE\$77config
	UninstallR77Config();

	return 0;
}