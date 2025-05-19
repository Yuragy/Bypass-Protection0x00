using System;
using GenEDRBypass.Core;

namespace GenEDRBypass.Tests
{
    public static class TestMemoryManager
    {
        public static void RunTests()
        {
            Console.WriteLine("Тестирование MemoryManager...");

            try
            {
                // Тест выделения памяти
                IntPtr memory = MemoryManager.AllocateExecutableMemory(1024);
                Console.WriteLine($"Память выделена по адресу: {memory}");

                // Тест изменения защиты памяти
                MemoryManager.ProtectMemory(memory, 1024, 0x20);
                Console.WriteLine("Защита памяти успешно изменена.");

                // Тест освобождения памяти
                MemoryManager.FreeMemory(memory, 1024);
                Console.WriteLine("Память успешно освобождена.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Ошибка в MemoryManager: {ex.Message}");
            }
        }
    }
}
