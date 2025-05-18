# Shellcode Injection with Syscalls

Proof of concept Windows shellcode injector to bypass AV and EDR. Currently uses assmebly partially created with the help of [Syswhispers](https://github.com/jthuraisamy/SysWhispers) to load system call numbers and make calls. Due to the outdated nature of the project, I had to add the last batch of Windows installations myself to make it compatible with all modern Windows versions -- Starting from the first release of Windows 10 (1507) to the most recent release of Windows 11 (23H2).


## Features

- Decrypts XOR shellcodes and injects into running process with passed PID
- Use of Windows kernel timer call locations to syscalls through them (NtCreateTimer, NtOpenTimer, NtSetTimer, NtQueryTimer, NtCancelTimer)
- Makes syscalls directly through assembly, avoiding the use of NTdll or Windows API to make calls   
- Built with x64 assmebly, Windows API, and standard C++ libraries

## Getting Started

Follow these simple steps to setup your environment and compile the injector:

### Prerequisites

1. Shellcode must be XOR encrypted BEFORE you paste into code, or comment out decryption call (line 31)
2. Visual Studio or another way to compile and link C++ with assembly (see compiling below for more details)

### Installation

1. Clone or download the repository to your local machine

2. Create a new C++ Visual Studio project in the directory

3. Open main.cpp and syscalls.asm as source files and wonka.h as a header file  
   
4. Make changes to necessary components (shellcode, XOR key)

5. Follow compilation steps

### Compiling

1. Right click on project name in Solution Explorer and click Build Dependencies --> Build Customizations
  
2. Check "masm" box and click Okay (see below)

   ![image](https://github.com/maxbarkouras/Syscall-Shellcode-Injector/assets/40187297/50cb96f4-3304-4d5f-ba76-9c5f592eace0)

3. Right click syscalls.asm file in Solutions Explorer and click Properties
   
4. Set "Excluded From Build" to No, "Content" to Yes, and "Item Type" to Microsoft Macro Assembler (see below)

   ![image](https://github.com/maxbarkouras/Syscall-Shellcode-Injector/assets/40187297/bb34f9d9-1187-46c2-8e1a-2f38479b1435)

5. Ready to Compile!

---

Done!
