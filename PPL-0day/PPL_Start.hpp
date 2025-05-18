#pragma once
#include <Windows.h>
#include "config.h"
#include "kernel_defs.h" // helper header for kernel defs

namespace PPL_Start
{
    using namespace kernel; // kernel_defs.h

    VOID BTE(BOOL bReturn, LPCWSTR lpwzTrace)
    {
        if (bReturn == FALSE)
        {
            ILog("Failed: %ls: %d (%x)\n", lpwzTrace, GetLastError(), GetLastError());
        }
        return;
    }

    DWORD GetProcessIntegrityLevel(HANDLE hProcess)
    {
        HANDLE hToken;

        DWORD dwLengthNeeded;
        DWORD dwError = ERROR_SUCCESS;

        PTOKEN_MANDATORY_LABEL pTIL = NULL;
        LPWSTR pStringSid;
        DWORD dwIntegrityLevel;

        if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
        {
            // Get the Integrity level.
            if (!GetTokenInformation(hToken, TokenIntegrityLevel,
                NULL, 0, &dwLengthNeeded))
            {
                dwError = GetLastError();
                if (dwError == ERROR_INSUFFICIENT_BUFFER)
                {
                    pTIL = (PTOKEN_MANDATORY_LABEL)LocalAlloc(0,
                        dwLengthNeeded);
                    if (pTIL != NULL)
                    {
                        if (GetTokenInformation(hToken, TokenIntegrityLevel,
                            pTIL, dwLengthNeeded, &dwLengthNeeded))
                        {
                            dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid,
                                (DWORD)(UCHAR)(*GetSidSubAuthorityCount(pTIL->Label.Sid) - 1));

                            if (dwIntegrityLevel < SECURITY_MANDATORY_MEDIUM_RID)
                            {
                                CloseHandle(hToken);
                                // Low Integrity
                                return INTEGRITY_LOW;
                            }
                            else if (dwIntegrityLevel < SECURITY_MANDATORY_HIGH_RID)
                            {
                                CloseHandle(hToken);
                                // Medium Integrity
                                return INTEGRITY_MEDIUM;
                            }
                            else if (dwIntegrityLevel < SECURITY_MANDATORY_SYSTEM_RID)
                            {
                                CloseHandle(hToken);
                                // High Integrity
                                return INTEGRITY_HIGH;
                            }
                            else if (dwIntegrityLevel >= SECURITY_MANDATORY_SYSTEM_RID)
                            {
                                CloseHandle(hToken);
                                // System Integrity
                                return INTEGRITY_SYSTEM;
                            }

                            else
                            {
                                CloseHandle(hToken);
                                return INTEGRITY_UNKNOWN;
                            }

                        }
                        LocalFree(pTIL);
                    }
                }
            }
            CloseHandle(hToken);
        }
        return INTEGRITY_UNKNOWN;
    }

    BOOL SetTokenIntegrityLevel(HANDLE& hToken, DWORD dwIntegrityLevel)
    {

        return INTEGRITY_UNKNOWN;
    }

    bool SetThreadProcessPrivilege(LPCWSTR PrivilegeName, bool Enable)
    {
        HANDLE Token;
        TOKEN_PRIVILEGES TokenPrivs;
        LUID TempLuid;
        bool Result;

        if (!LookupPrivilegeValueW(NULL, PrivilegeName, &TempLuid))  return false;

        if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &Token))
        {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &Token))  return false;
        }

        TokenPrivs.PrivilegeCount = 1;
        TokenPrivs.Privileges[0].Luid = TempLuid;
        TokenPrivs.Privileges[0].Attributes = (Enable ? SE_PRIVILEGE_ENABLED : 0);

        Result = (AdjustTokenPrivileges(Token, FALSE, &TokenPrivs, 0, NULL, NULL) && ::GetLastError() == ERROR_SUCCESS);

        // Even if AdjustTokenPrivileges returns TRUE, it may not have succeeded
        // check last error top confirm
        if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
        {
            ILog(" Unable to set privilege: %S Error: %d \n", PrivilegeName, GetLastError());
            CloseHandle(Token);
            return FALSE;
        }

        CloseHandle(Token);

        return Result;
    }

    BOOL GetTokenFromPID(HANDLE& hToken, DWORD pid, TOKEN_TYPE duptype, DWORD accessmode = TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY)
    {
        HANDLE tempproc;
        HANDLE tokenhandle, tokenhandle2;

        // Enable SeDebugPrivilege.
        SetThreadProcessPrivilege(L"SeDebugPrivilege", true);

        // Open a handle to the process.
        tempproc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (tempproc == NULL)  tempproc = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

        if (tempproc == NULL)
        {
            ILog("OpenProcess failed with error %d (0x%X) for PID %d (0x%X)\n", GetLastError(), GetLastError(), pid, pid);

            return FALSE;
        }

        if (!OpenProcessToken(tempproc, accessmode, &tokenhandle) && (!(accessmode & TOKEN_QUERY_SOURCE) || !OpenProcessToken(tempproc, accessmode & ~TOKEN_QUERY_SOURCE, &tokenhandle)))
        {
            ILog("OpenProcessToken failed with error %d (0x%X) for PID %d (0x%X)\n", GetLastError(), GetLastError(), pid, pid);

            ::CloseHandle(tempproc);

            return FALSE;
        }

        CloseHandle(tempproc);

        if (!(accessmode & TOKEN_DUPLICATE))  tokenhandle2 = tokenhandle;
        else
        {
            SECURITY_ATTRIBUTES secattr = { 0 };
            secattr.nLength = sizeof(secattr);
            secattr.bInheritHandle = FALSE;
            secattr.lpSecurityDescriptor = NULL;

            if (!DuplicateTokenEx(tokenhandle, MAXIMUM_ALLOWED, &secattr, SecurityImpersonation, duptype, &tokenhandle2))
            {
                ILog("DuplicateTokenEx failed with error %d (0x%X) for PID %d (0x%X)\n", GetLastError(), GetLastError(), pid, pid);

                CloseHandle(tokenhandle);

                return FALSE;
            }

            CloseHandle(tokenhandle);
        }

        hToken = tokenhandle2;
        return TRUE;
    }

    BOOL EnableSystemPrivileges()
    {
        if (!SetThreadProcessPrivilege(L"SeBackupPrivilege", true))
        {
            ILog("Failed to enable SeBackupPrivilege: %d\n", GetLastError());

            return FALSE;
        }

        if (!SetThreadProcessPrivilege(L"SeRestorePrivilege", true))
        {
            ILog("Failed to enable SeRestorePrivilege: %d\n", GetLastError());

            return FALSE;
        }

        if (!SetThreadProcessPrivilege(L"SeIncreaseQuotaPrivilege", true))
        {
            ILog("Failed to enable SeIncreaseQuotaPrivilege: %d\n", GetLastError());

            return FALSE;
        }

        if (!SetThreadProcessPrivilege(L"SeAssignPrimaryTokenPrivilege", true))
        {
            ILog("Failed to enable SeAssignPrimaryTokenPrivilege: %d\n", GetLastError());

            return FALSE;
        }

        if (!SetThreadProcessPrivilege(L"SeTcbPrivilege", true))
        {
            ILog("Failed to enable SeTcbPrivilege: %d\n", GetLastError());

            return FALSE;
        }
        
        if (!SetThreadProcessPrivilege(L"SeDebugPrivilege", true))
        {
            ILog("Failed to enable SeDebugPrivilege: %d\n", GetLastError());

            return FALSE;
        }

        return TRUE;
    }



    DWORD GetSID(LPCWSTR lptszUserName, PSID pSid, PDWORD pdwSize)
    {
        DWORD dwError;
        LPWSTR lptszDomainName;
        DWORD dwDomainNameLen;
        SID_NAME_USE snu;
        LPWSTR lptszSid;
        BOOL bRet;

        dwError = ERROR_SUCCESS;

        *pdwSize = 0;
        dwDomainNameLen = 0;
        lptszDomainName = NULL;
        bRet = LookupAccountName(NULL, lptszUserName, NULL, pdwSize, NULL, &dwDomainNameLen, &snu);
        dwError = GetLastError();
        pSid = new BYTE[*pdwSize];
        lptszDomainName = new TCHAR[dwDomainNameLen + 1];
        SecureZeroMemory(lptszDomainName, sizeof(TCHAR) * (dwDomainNameLen + 1));
        SecureZeroMemory(pSid, *pdwSize);
        bRet = LookupAccountNameW(NULL, lptszUserName, pSid, pdwSize, lptszDomainName, &dwDomainNameLen, &snu);
        dwError = GetLastError();

        delete[] lptszDomainName;
        return dwError;
    }

    BOOL WINAPI CreateProcessSuspended(
        _In_ LPCWSTR& lpApplicationPath,
        _Out_ HANDLE& hProcess,
        _Out_ HANDLE& hMainThread
    )
    {
        STARTUPINFO si = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi = { 0 };
        BOOL bSuccess = FALSE;
        LPWSTR commandLine = new WCHAR[MAX_PATH];
        wcscpy(commandLine, lpApplicationPath);

        ILog("Using command line %ls\n", commandLine);

        // Initialize PROCESS_INFORMATION
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

        // Initialize STARTUPINFO
        ZeroMemory(&si, sizeof(STARTUPINFO));

        // Start the child process. 
        bSuccess = CreateProcess(
            NULL,   // No module name (use command line)
            commandLine,        // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            CREATE_SUSPENDED, // Creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si,            // Pointer to STARTUPINFO structure
            &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
        );

        delete[] commandLine;

        if (bSuccess)
        {
            hProcess = pi.hProcess;
            hMainThread = pi.hThread;
            return TRUE;
        }

        return FALSE;
    }

    BOOL SetNoWriteUpLabel(HANDLE hToken, WELL_KNOWN_SID_TYPE stIntegrityLevel)
    {
        ULONG cbSid = GetSidLengthRequired(1);

        TOKEN_MANDATORY_LABEL tml = { { 0 } };
        SecureZeroMemory(&tml, sizeof(TOKEN_MANDATORY_LABEL));

        ULONG dwError = NOERROR;

        if (CreateWellKnownSid(stIntegrityLevel, 0, tml.Label.Sid, &cbSid))
        {
            SECURITY_DESCRIPTOR sd;
            ULONG cbAcl = sizeof(ACL) + FIELD_OFFSET(SYSTEM_MANDATORY_LABEL_ACE, SidStart) + cbSid;
            PACL Sacl = (PACL)sizeof(cbAcl);

            if (!InitializeAcl(Sacl, cbAcl, ACL_REVISION) ||
                !AddMandatoryAce(Sacl, ACL_REVISION, 0, SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, tml.Label.Sid) ||
                !InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) ||
                !SetSecurityDescriptorSacl(&sd, TRUE, Sacl, FALSE) ||
                !SetKernelObjectSecurity(hToken, LABEL_SECURITY_INFORMATION, &sd) ||
                !SetTokenInformation(hToken, TokenIntegrityLevel, &tml, sizeof(tml)))
            {
                dwError = GetLastError();
            }
        }

        if (dwError == NO_ERROR)
            return TRUE;
        else
            return FALSE;
    }

    BOOL SetProcessLimitedToken(
        _In_ HANDLE hProcess,
        _In_ HANDLE hMainThread
    )
    {
        NtWrapper ntdll;
        LUID luid;
        LookupPrivilegeValueW(0, SE_ASSIGNPRIMARYTOKEN_NAME, &luid);
        TOKEN_PRIVILEGES privs;
        privs.PrivilegeCount = 1;
        privs.Privileges[0].Luid = luid;
        privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!EnableSystemPrivileges())
        {
            wprintf(L"Error setting token privileges: 0x%08x\n", GetLastError());
            return FALSE;
        }

        LUID_AND_ATTRIBUTES luidAttr = { luid, 0 };

        // Get the DISABLE_MAX_PRIVILEGE duplicate primary token of our process
        HANDLE newToken = NULL;

        if (!GetTokenFromPID(newToken, GetCurrentProcessId(), TokenPrimary))
        {
            ILog("Error getting token from PID: 0x%08x\n", GetLastError());
            return FALSE;
        }

        if (newToken == NULL)
        {
            ILog("Error getting token from PID: 0x%08x\n", GetLastError());
            return FALSE;
        }

        // Create a mandatory low integrity SID

        //SetNoWriteUpLabel(newToken, WinLowLabelSid);

        SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_MANDATORY_LABEL_AUTHORITY;
        PSID pSid = NULL;
        AllocateAndInitializeSid(&SIDAuth, 1, SECURITY_MANDATORY_LOW_RID, 0, 0, 0, 0, 0, 0, 0, &pSid);

        // Change the integrity level of the duplicate token
        TOKEN_MANDATORY_LABEL tml;
        tml.Label.Attributes = SE_GROUP_INTEGRITY;
        tml.Label.Sid = pSid;
        if (!SetTokenInformation(newToken, TokenIntegrityLevel, &tml, sizeof(tml)))
        {
            ILog("Failed to set token information: %d\n", ::GetLastError());
        }

        // Initialize the kernel object attributes
        PROCESS_ACCESS_TOKEN tokenInfo;
        tokenInfo.Token = newToken;
        tokenInfo.Thread = 0;

        // Get a handle to ntdll
        HMODULE hmntdll = LoadLibrary(L"ntdll.dll");

        NTSTATUS status;

        KERNEL_PROCESS_INFORMATION_CLASS infoClass = NtProcessAccessToken;

        // And a pointer to the NtSetInformationProcess function
        //NtSetInformationProcess setInfo = (NtSetInformationProcess)GetProcAddress(ntdll, "NtSetInformationProcess");
        NTSTATUS setInfoResult = ntdll.NtSetInformationProcess(hProcess, (PROCESSINFOCLASS)infoClass, &tokenInfo, sizeof(PROCESS_ACCESS_TOKEN));
        if (setInfoResult < 0)
        {
            wprintf(L"Error setting token: 0x%08x\n", setInfoResult);
            return FALSE;
        }

        // Cleanup
        FreeLibrary(hmntdll);
        CloseHandle(newToken);

        // Resume the process
        ResumeThread(hMainThread);

        return TRUE;
    }
}
