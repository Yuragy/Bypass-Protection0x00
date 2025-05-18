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
1. **Auto-Color**  
   A polymorphic obfuscation toolkit that uses color-based encoding to evade static detection.  

2. **BypassAV**  
   Automated framework for disabling or bypassing Windows antivirus engines via API hooking and patching.  

3. **CallstackSpoofingPOC**  
   Proof-of-concept demonstrating call-stack spoofing techniques to defeat Control-Flow Integrity (CFI).  

4. **DSC**  
   Driver Signature Check bypass module enabling the loading of unsigned kernel drivers on Windows.  

5. **EfiGuard**  
   Exploit for bypassing UEFI firmware protections and executing unauthorized code during boot.  

6. **ElfDoor-gcc**  
   Linux kernel module loader that injects unsigned ELF objects into kernel space to bypass module signing.  

7. **Hanshell**  
   Shellcode packer/loader with dynamic encryption and anti-analysis features.  

8. **PPL-0day**  
   Proof-of-concept exploit targeting Windows Protected Process Light (PPL) to bypass PPL enforcement.  

9. **Shellcode-Injector**  
   Generic shellcode injection framework supporting reflective injection and process hollowing.  

10. **landrun**  
    Payload loader that leverages custom containerization techniques for stealth execution.  

11. **power-killEDR_AV**  
    Utility to terminate EDR/AV processes by exploiting high-privilege system calls.  

12. **zapper**  
    Cleanup tool for erasing logs, disabling tamper protections, and removing forensic traces.  

## 🚫 Disclaimer

This repository is provided for **educational purposes only** and intended for **authorized security research**.
Use of these materials in unauthorized or illegal activities is **strictly prohibited**.
