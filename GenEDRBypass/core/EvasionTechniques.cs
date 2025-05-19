using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;

namespace GenEDRBypass.Core
{
    public static class EvasionTechniques
    {
        [DllImport("kernel32.dll")]
        private static extern bool IsDebuggerPresent();

        [DllImport("kernel32.dll")]
        private static extern bool CheckRemoteDebuggerPresent(IntPtr hProcess, ref bool isDebuggerPresent);

        // Проверка на наличие отладчика
        public static void AntiDebugCheck()
        {
            Console.WriteLine("Проверка на наличие отладчика...");

            if (IsDebuggerPresent())
                Environment.Exit(1); // Завершаем выполнение, если обнаружен дебаггер

            bool isRemoteDebuggerPresent = false;
            CheckRemoteDebuggerPresent(Process.GetCurrentProcess().Handle, ref isRemoteDebuggerPresent);
            if (isRemoteDebuggerPresent)
                Environment.Exit(1); // Завершаем выполнение, если обнаружен удаленный дебаггер
        }

        // Проверки на запуск в песочнице
        public static void AntiSandboxChecks()
        {
            Console.WriteLine("Проверка на запуск в песочнице...");

            if (System.Windows.Forms.Screen.PrimaryScreen.Bounds.Width < 800 || System.Windows.Forms.Screen.PrimaryScreen.Bounds.Height < 600)
            {
                Environment.Exit(1); // Завершаем выполнение, если обнаружена песочница
            }
        }

        // Запутывание логики выполнения
        public static void ObfuscateExecution()
        {
            Console.WriteLine("Запутывание выполнения...");

            for (int i = 0; i < 10; i++)
            {
                Thread.Sleep(100); // Небольшие задержки для обхода анализа
            }
        }
    }
}
