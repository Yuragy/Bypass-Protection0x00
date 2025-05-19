using System;
using System.IO;

namespace GenEDRBypass.Utils
{
    public static class Logger
    {
        private static readonly string LogFilePath = "GenEDRBypass.log";

        // Метод для логирования сообщений
        public static void Log(string message)
        {
            string formattedMessage = $"[{DateTime.Now}] {message}";

            // Выводим в консоль
            Console.WriteLine(formattedMessage);

            // Сохраняем в файл
            SaveToFile(formattedMessage);
        }

        // Метод для сохранения сообщения в файл
        private static void SaveToFile(string message)
        {
            try
            {
                using (StreamWriter writer = new StreamWriter(LogFilePath, true))
                {
                    writer.WriteLine(message);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Ошибка при записи логов в файл: {ex.Message}");
            }
        }

        // Метод для логирования ошибок
        public static void LogError(string error)
        {
            Log($"[ОШИБКА] {error}");
        }

        // Метод для логирования информации
        public static void LogInfo(string info)
        {
            Log($"[ИНФО] {info}");
        }
    }
}
