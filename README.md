# EDR & AV Bypass Arsenal

**Comprehensive collection of tools, patches, and techniques for evading modern EDR, AV, and other defenses**

## Functional Specifics:
- Obfuscation & Polymorphism      
- AV/EDR Bypass                   
- Control-Flow Spoofing           
- river Signature Bypass         
- EFI/Boot Protection Bypass      
- Shellcode Injection & Loaders   
- Defense Process Termination     

## Repository Structure
1️⃣ **Auto-Color**  
   A polymorphic obfuscation toolkit that uses color-based encoding to evade static detection.  

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

## 🚫 Disclaimer

This repository is provided for **educational purposes only** and intended for **authorized security research**.
Use of these materials in unauthorized or illegal activities is **strictly prohibited**.
