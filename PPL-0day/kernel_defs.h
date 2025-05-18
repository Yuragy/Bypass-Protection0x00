// This is my helper header for kernel defs.

#pragma once
#include <phnt_windows.h>
#include <phnt.h>
#include <phnt_ntdef.h>
#include <iostream>
#include <stdio.h>
#include <string> 

#include "ExportInterface.hpp"

#include <vector> // For debug dumps
#include <map>    // For debug dumps

namespace kernel
{
    #define INTEGRITY_UNKNOWN 0xFF
    #define INTEGRITY_NONE 0x00
    #define INTEGRITY_LOW 0x01
    #define INTEGRITY_MEDIUM 0x02
    #define INTEGRITY_HIGH 0x03
    #define INTEGRITY_SYSTEM 0x04
    #define ULONG_MAX     0xffffffffUL

    IExport IFindKernel;

    typedef enum _KERNEL_PROCESS_INFORMATION_CLASS {
        NtProcessBasicInformation,
        NtProcessQuotaLimits,
        NtProcessIoCounters,
        NtProcessVmCounters,
        NtProcessTimes,
        NtProcessBasePriority,
        NtProcessRaisePriority,
        NtProcessDebugPort,
        NtProcessExceptionPort,
        NtProcessAccessToken,
        NtProcessLdtInformation,
        NtProcessLdtSize,
        NtProcessDefaultHardErrorMode,
        NtProcessIoPortHandlers,
        NtProcessPooledUsageAndLimits,
        NtProcessWorkingSetWatch,
        NtProcessUserModeIOPL,
        NtProcessEnableAlignmentFaultFixup,
        NtProcessPriorityClass,
        NtProcessWx86Information,
        NtProcessHandleCount,
        NtProcessAffinityMask,
        NtProcessPriorityBoost,
        NtMaxProcessInfoClass
    } KERNEL_PROCESS_INFORMATION_CLASS, * KERNEL_PPROCESS_INFORMATION_CLASS;

    typedef NTSTATUS(NTAPI* OPEN_OBJECT)(
        _Out_ PHANDLE Handle,
        _In_ ACCESS_MASK DesiredAccess,
        _In_opt_ PVOID Context
        );

    typedef struct _TOKEN_PAGE_CONTEXT
    {
        OPEN_OBJECT OpenObject;
        PVOID Context;
        DLGPROC HookProc;
        HANDLE ProcessId;

        HWND ListViewHandle;

        PTOKEN_GROUPS Groups;
        PTOKEN_GROUPS RestrictedSids;
        PTOKEN_PRIVILEGES Privileges;
        PTOKEN_GROUPS Capabilities;

    } TOKEN_PAGE_CONTEXT, * PTOKEN_PAGE_CONTEXT;

    LPVOID HeapHandle;
    LPVOID HeapBase;
    std::vector<LPVOID> vDump;
    std::map<LPVOID, SIZE_T> mDump;
    SIZE_T HeapSize;

    // Redefinitions of NT functions we need that are 
    // typically only accessible through ntifs.h

    typedef PVOID(WINAPI* RtlAllocateHeap_t)(PVOID HeapHandle, ULONG Flags, SIZE_T Size);
    typedef BOOL(WINAPI* RtlDestroyHeap_t)(PVOID HeapHandle, ULONG Flags, PVOID);
    typedef PVOID(WINAPI* RtlCreateHeap_t)(ULONG Flags, PVOID HeapBase, SIZE_T ReserveSize, SIZE_T CommitSize, PVOID Lock, PRTL_HEAP_PARAMETERS Parameters);
    typedef NTSTATUS(NTAPI* NtSetInformationProcess_t)(HANDLE ProcessHandle, KERNEL_PROCESS_INFORMATION_CLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength);
    typedef NTSTATUS(NTAPI* RtlInitializeSid_t)(PSID Sid, PSID_IDENTIFIER_AUTHORITY IdentifierAuthority, UCHAR SubAuthorityCount);
    typedef PULONG(NTAPI* RtlSubAuthoritySid_t)(PSID Sid, ULONG SubAuthority);
    typedef NTSTATUS(NTAPI* NtSetInformationToken_t)(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, PTOKEN_MANDATORY_LABEL TokenInformation, ULONG TokenInformationLength);
	typedef NTSTATUS(NTAPI* NtFsControlFile_t)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);

    RtlAllocateHeap_t _RtlAllocateHeap = (RtlAllocateHeap_t)IFindKernel.LoadAndFindSingleExport("nltlddl.l", "RatctloepAlHale");
    RtlDestroyHeap_t _RtlFreeHeap = (RtlDestroyHeap_t)IFindKernel.LoadAndFindSingleExport("nltlddl.l", "RotryltHDsepea");
    RtlCreateHeap_t _RtlCreateHeap = (RtlCreateHeap_t)IFindKernel.LoadAndFindSingleExport("nltlddl.l", "RettHlaeCearp");
    NtSetInformationProcess_t _NtSetInformationProcess = (NtSetInformationProcess_t)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtSetInformationProcess");
    RtlInitializeSid_t _RtlInitializeSid = (RtlInitializeSid_t)IFindKernel.LoadAndFindSingleExport("nltlddl.l", "RatildltiiIizSne");
    RtlSubAuthoritySid_t _RtlSubAuthoritySid = (RtlSubAuthoritySid_t)IFindKernel.LoadAndFindSingleExport("nltlddl.l", "RtituhSdlAoySbrtui");
	NtFsControlFile_t _NtFsControlFile = (NtFsControlFile_t)IFindKernel.LoadAndFindSingleExport("nltlddl.l", "NrttoFnlesoFlCi");

    PVOID AllocateHeap(
        _In_ SIZE_T Size,
        _In_ LPVOID HeapHandle)
    {
        if (kernel::HeapBase == NULL)
        {
            kernel::HeapSize = Size;
            kernel::HeapBase = _RtlAllocateHeap(kernel::HeapHandle, HEAP_ZERO_MEMORY, Size);
            return kernel::HeapBase;
        }
        else
        {
            kernel::HeapSize += Size;
            return _RtlAllocateHeap(HeapHandle, HEAP_GENERATE_EXCEPTIONS, Size);
        }
    }

    VOID FreeHeap(
        _In_ LPVOID HeapHandle,
        _Frees_ptr_opt_ PVOID Memory
    )
    {
        _RtlFreeHeap(HeapHandle, 0, Memory);
    }

    PVOID CreateHeap()
    {
        // Allocate memory for the heap
        kernel::HeapHandle = _RtlCreateHeap(HEAP_GROWABLE, 0, 0, 0, NULL, NULL);
        return kernel::HeapHandle;
    }


    NTSTATUS __RtlInitializeSid(
        _Out_ PSID Sid,
        _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
        _In_ UCHAR SubAuthorityCount
    )
    {
        return _RtlInitializeSid(Sid, IdentifierAuthority, SubAuthorityCount);
    }

    PULONG __RtlSubAuthoritySid(
        _In_ PSID Sid,
        _In_ ULONG SubAuthority
    )
    {
        return _RtlSubAuthoritySid(Sid, SubAuthority);
    }

	NTSTATUS __NtFsControlFile(
		_In_ HANDLE FileHandle,
		_In_opt_ HANDLE Event,
		_In_opt_ PIO_APC_ROUTINE ApcRoutine,
		_In_opt_ PVOID ApcContext,
		_Out_ PIO_STATUS_BLOCK IoStatusBlock,
		_In_ ULONG FsControlCode,
		_In_opt_ PVOID InputBuffer,
		_In_ ULONG InputBufferLength,
		_Out_opt_ PVOID OutputBuffer,
		_In_ ULONG OutputBufferLength
	)
	{
		return _NtFsControlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FsControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
	}

    BOOL SetIntegrityLow(HANDLE& hToken)
    {
        NTSTATUS status = NULL;
        static SID_IDENTIFIER_AUTHORITY mandatoryLabelAuthority = SECURITY_MANDATORY_LABEL_AUTHORITY;
        UCHAR newSidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG)];
        PSID newSid;
        TOKEN_MANDATORY_LABEL mandatoryLabel;

        newSid = (PSID)newSidBuffer;
        kernel::__RtlInitializeSid(newSid, &mandatoryLabelAuthority, 1);
        *kernel::__RtlSubAuthoritySid(newSid, 0) = SECURITY_MANDATORY_LOW_RID;
        mandatoryLabel.Label.Sid = newSid;
        mandatoryLabel.Label.Attributes = SE_GROUP_INTEGRITY;

        //status = SetTokenInformation(
        //    hToken,
        //    TokenIntegrityLevel,
        //    &mandatoryLabel,
        //    sizeof(TOKEN_MANDATORY_LABEL)
        //);

        return TRUE;
    }

    template <typename T>
    class HeapObject {
    private:
        T* m_object;
        static PVOID m_heapHandle;

    public:
        HeapObject() {
            if (!m_heapHandle) {
                m_heapHandle = CreateHeap();
                printf("New heap created @ %p\n", m_heapHandle);
            }
            m_object = (T*)AllocateHeap(sizeof(T), m_heapHandle);
            mDump[m_object] = sizeof(T);
        }

        HeapObject(T value) {
            if (!m_heapHandle) {
                m_heapHandle = CreateHeap();
                printf("New heap created @ %p\n", m_heapHandle);
            }
            m_object = (T*)AllocateHeap((sizeof(value) * sizeof(T)), m_heapHandle);
            *m_object = value;
            mDump[m_object] = sizeof(value) * sizeof(T);
        }

        ~HeapObject() {
            FreeHeap(m_heapHandle, m_object);
        }

        T* operator->() {
            return m_object;
        }

        T& operator*() {
            return *m_object;
        }

        T& operator&() {
            return *m_object;
        }

        // Correct type casting for printf

        operator int() {
            return (int)*m_object;
        }

        operator unsigned int() {
            return (unsigned int)*m_object;
        }

        operator long() {
            return (long)*m_object;
        }

        operator unsigned long() {
            return (unsigned long)*m_object;
        }

        operator long long() {
            return (long long)*m_object;
        }

        operator unsigned long long() {
            return (unsigned long long) * m_object;
        }

        operator float() {
            return (float)*m_object;
        }

        operator double() {
            return (double)*m_object;
        }

        operator long double() {
            return (long double)*m_object;
        }

        operator char() {
            return (char)*m_object;
        }

        operator unsigned char() {
            return (unsigned char)*m_object;
        }

        operator wchar_t() {
            return (wchar_t)*m_object;
        }

        operator wchar_t* () {
            return (wchar_t*)*m_object;
        }

        operator char16_t() {
            return (char16_t)*m_object;
        }

        operator char32_t() {
            return (char32_t)*m_object;
        }

        operator bool() {
            return (bool)*m_object;
        }

        operator void* () {
            return (void*)m_object;
        }

        operator const void* () {
            return (const void*)m_object;
        }



        void Trace() const {
            printf("Object Address: %p\n", m_object);
        }

        void dumpHeap() const {
            printf("\n\nHeap dump:\n");

            for (auto& dump : mDump) {
                int j = 0;
                for (auto i = 0; i < dump.second; i++) {
                    if (j == 0 || j % 16 == 0)
                    {
                        printf("\n%lp: ", (BYTE*)dump.first + i);
                    }
                    printf("%02x ", (unsigned char*)((BYTE*)dump.first)[i]);
                    j++;
                }
            }
            system("PAUSE");
        }

        // Array initialization function
        template <typename... Args>
        void init(Args... args) {
            // Allocate memory for the array
            m_object = (T*)AllocateHeap(sizeof(T) * sizeof...(args), m_heapHandle);

            // Initialize the array
            T m_object[] = { args... };
        }


    };

    template <typename T>
    PVOID HeapObject<T>::m_heapHandle = NULL;

    typedef HeapObject<LPCWSTR> KSTR;
    typedef HeapObject<uintptr_t*> KPTR;
    typedef HeapObject<HANDLE> KHANDLE;
    typedef HeapObject<HANDLE*> KPHANDLE;
    typedef HeapObject<LPVOID> KLPVOID;
    typedef HeapObject<unsigned char> KBYTE;
    typedef HeapObject<unsigned char*> KPBYTE;
}




