using System;
using GenEDRBypass.Core;

namespace GenEDRBypass.Tests
{
    public static class TestEvasionTechniques
    {
        public static void RunTests()
        {
            Console.WriteLine("Тестирование EvasionTechniques...");

            try
            {
                // Тест антиотладочной проверки
                EvasionTechniques.AntiDebugCheck();
                Console.WriteLine("Проверка на отладчик пройдена.");

                // Тест проверки на песочницу
                EvasionTechniques.AntiSandboxChecks();
                Console.WriteLine("Проверка на песочницу пройдена.");

                // Тест запутывания выполнения
                EvasionTechniques.ObfuscateExecution();
                Console.WriteLine("Запутывание выполнения прошло успешно.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Ошибка в EvasionTechniques: {ex.Message}");
            }
        }
    }
}
