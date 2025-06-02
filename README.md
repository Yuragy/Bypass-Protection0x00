<p align="center">
	<i><u>EDR & AV Bypass Arsenal</i></u>
</p>
<p align="center">
  <img src="https://github.com/Yuragy/Bypass-Protection0x00/blob/main/reso/adobe.jpg">
</p> 

**Comprehensive collection of tools, patches, and techniques for evading modern EDR, AV, and other defenses.**
All the tools in this repository are a collection that continues to grow, the author's contacts are listed on the inside of each tool if possible. 
This project is intended for security researchers and students. 

## 🚫 Disclaimer

This repository is provided for **educational purposes only** and intended for **authorized security research**.
Use of these materials in unauthorized or illegal activities is **strictly prohibited**.

## Functional Specifics ✅

- Obfuscation & Polymorphism  
- AV/EDR Bypass  
- Windows SmartScreen Bypass  
- C2 Proxy Relaying  
- Control-Flow Spoofing  
- Driver Signature Bypass  
- EFI/Boot Protection Bypass  
- PE Infector & Binary Patching  
- Shellcode Injection & Loaders  
- APC-Based Code Injection  
- Shellcode Mutation  
- Defense Process Termination    

## Repository Structure
1️⃣ **Auto-Color**  

    Polymorphic obfuscation toolkit that uses color-based encoding to evade static detection.  

2️⃣ **BypassAV**  

    Automated framework for disabling or bypassing Windows antivirus engines via API hooking and patching.  

3️⃣ **CallstackSpoofingPOC**  

    Proof-of-concept demonstrating call-stack spoofing techniques to defeat Control-Flow Integrity (CFI).  

4️⃣ **DSC**  

    Driver Signature Check bypass module enabling the loading of unsigned kernel drivers on Windows.  

5️⃣ **EfiGuard**  

    Exploit for bypassing UEFI firmware protections and executing unauthorized code during boot.  

6️⃣ **ElfDoor-gcc**  

    Linux kernel module loader that injects unsigned ELF objects into kernel space to bypass module signing.  

7️⃣ **Hanshell**  

    Shellcode packer/loader with dynamic encryption and anti-analysis features.  

8️⃣ **PPL-0day**

    Proof-of-concept exploit targeting Windows Protected Process Light (PPL) to bypass PPL enforcement.  

9️⃣ **Shellcode-Injector**

    Generic shellcode injection framework supporting reflective injection and process hollowing.  

1️⃣0️⃣ **Landrun**  

    Payload loader that leverages custom containerization techniques for stealth execution.  

1️⃣1️⃣ **Power-killEDR_AV**  

    Utility to terminate EDR/AV processes by exploiting high-privilege system calls.  

1️⃣2️⃣ **Zapper**  

    Cleanup tool for erasing logs, disabling tamper protections, and removing forensic traces.  
    
1️⃣3️⃣ **APC-Injection**  

    Leverages Windows Asynchronous Procedure Calls to queue and execute arbitrary code in remote processes for stealthy injection.

1️⃣4️⃣ **Bypass-EDR**  

    Collection of techniques and scripts to disable or evade common Endpoint Detection & Response platforms at runtime.

1️⃣5️⃣ **Bypass-Smartscreen**  

    Implements methods to circumvent Windows SmartScreen application reputation checks and “unknown publisher” warnings.

1️⃣6️⃣ **Google Script Proxy**  

    Command-and-control proxy using Google Apps Script to relay C2 traffic over Google’s infrastructure.

1️⃣7️⃣ **PE-infector**  

    Injects custom shellcode or payloads into Portable Executable files, modifying headers and sections for stealthy distribution.

1️⃣8️⃣ **PandaLoader**  

    Payload loader that uses API hooking and reflective techniques to hide code in protected or monitored processes.

1️⃣9️⃣ **Shellcode-Loader**  

    Simple framework for allocating memory, writing shellcode, and invoking it via various injection primitives (e.g., CreateRemoteThread).

2️⃣0️⃣ **Shellcode-Mutator**  

    Applies polymorphic transformations to raw shellcode—encryption, encoding, padding—to evade signature-based detection.

2️⃣1️⃣ **el84_injector**  

    ELF injector for Linux: attaches to a running process and maps arbitrary ELF segments into its memory space for execution.

2️⃣2️⃣ **AV\_Clean**

    Set of scripts and utilities for removing antivirus traces: stops services, deletes files and registry keys, and rolls back changes.

2️⃣3️⃣ **Byte**

    ZIP-bomb generator that creates ultra-compressed archives which expand into huge file sets to exhaust disk space, memory, or CPU resources.

2️⃣4️⃣ **Cryptolib**

    Common library of cryptographic primitives: encryption, hashing, and obfuscation routines for use in other tools.

2️⃣5️⃣ **Dump**

    Utility for dumping process and kernel memory (including LSASS) with support for compression and encryption of the output files.

2️⃣6️⃣ **DVUEFI**

    Educational platform and PoC suite for analyzing UEFI firmware vulnerabilities, with Secure Boot bypass techniques and integrity-check evasion.

2️⃣7️⃣ **GenEDRBypass**

    EDR-bypass generator: dynamically produces shellcode via msfvenom, applies XOR obfuscation, and includes anti-debug and anti-sandbox features.

2️⃣8️⃣ **Morpheus**

    Stealthy in-memory LSASS dumper: compresses memory dumps and exfiltrates them over obfuscated NTP-style UDP packets secured with RC4 and error-correction.

2️⃣9️⃣ **SecureUxTheme**

    Patch and loader for disabling signature checks in UxTheme.dll, allowing the installation of unsigned Windows themes.

3️⃣0️⃣ **TripleCross**

    Code injection framework leveraging COM objects to execute payloads in protected processes without direct API calls.

3️⃣1️⃣ **UEFISecureBoot**

    Scripts and PoCs for bypassing or disabling UEFI Secure Boot by chain-loading unsigned bootloaders and modifying firmware variables.

3️⃣2️⃣ **Vulnerable**

    Collection of intentionally vulnerable applications, drivers, and firmware images for practicing and demonstrating bypass techniques.

3️⃣3️⃣ **elf-infector**

    Linux ELF binary infector that injects custom shellcode into existing executables by modifying headers and segments for stealthy execution.

3️⃣4️⃣ **gnu-efi**

    Build scripts and headers for creating UEFI applications using GNU-EFI, simplifying Secure Boot testing.

3️⃣5️⃣ **injectAmsiBypass**

    Beacon Object File and standalone module that dynamically patches AMSI in memory to bypass script-scanning defenses.

3️⃣6️⃣ **kernel-callback**

    Kernel-mode injection primitive using Routine Callback, executing payloads in kernel context while bypassing user-mode hooks.

3️⃣7️⃣ **kernel-hardening-checker**

    Windows PatchGuard auditor that inspects driver-signature settings and reports potential bypass attack vectors.

3️⃣8️⃣ **lib**

    Shared libraries and utilities for process management, injection primitives, and obfuscation methods used across multiple tools.

3️⃣9️⃣ **mcuboot**

    Reference bootloader for microcontrollers with firmware-signature verification and chain-of-trust support for embedded systems.

4️⃣0️⃣ **phnt**

    Header only collection of Windows NT API definitions and internal structures for low-level system programming.

4️⃣1️⃣ **redlotus**

    Advanced in-memory loader with reflective loading and encrypted payload delivery to evade analysis.

4️⃣2️⃣ **rootkit**

    Kernel mode rootkit framework for hiding processes, inline hooking, and bypassing Event Tracing for Windows (ETW) on modern systems.

4️⃣3️⃣ **scripts**

    Helper scripts for building, deploying, and automating tools: compilation helpers and test C2-harnesses.

4️⃣4️⃣ **shim**

    Custom shim-DLL and loader mechanism to intercept application launches, patch imports, and bypass AppLocker/SmartScreen.
    
4️⃣5️⃣ **Nimbus**
    
    Contains a C# reflective-loader for .NET assemblies (EXE/DLL) that loads and immediately executes .NET applications in memory without creating temporary files on disk.
    
4️⃣6️⃣ **Shellcode-Hide**

    Set of tools for preparing and covertly executing shellcode on Windows, including loaders, encoders and encryptors
    
4️⃣7️⃣ **Safari 1day RCE Exploit**

    Exploit RCE vulnerability in WebKit/Safari running on certain versions of iOS and macOS.

