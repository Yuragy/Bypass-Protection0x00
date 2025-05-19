# EN
**GenEDRBypass** An advanced tool for bypassing EDR (Endpoint Detection and Response) systems and antivirus software by dynamically generating and injecting shellcode. This project is aimed at professionals in cybersecurity for ethical hacking, testing security defenses, and educational purposes.

## Features
- **Dynamic Shellcode Generation**: Automatically generates shellcode using `msfvenom`.
- **Obfuscation Techniques**: Employs XOR encryption and dynamic key rotation to avoid signature-based detection.
- **Memory Management**: Allocates and manipulates memory for executing the shellcode.
- **Anti-Debugging and Anti-Sandboxing**: Implements techniques to evade debuggers and sandbox environments.
- **Configurable Settings**: IP address and port for reverse connections are easily configurable.


## Getting Started
### Prerequisites
- .NET Core SDK
- `msfvenom` (Part of the Metasploit framework)
- Admin privileges for full functionality

### Installation
1. **Clone the repository**:
    ```bash
    git clone https://github.com/geniuszly/GenEDRBypass
    cd GenEDRBypass
    ```

2. **Install dependencies**:
    Make sure to have .NET Core installed on your machine. You can download it [here](https://dotnet.microsoft.com/download).

### Usage
1. **Configure `Settings.cs`**:
   Open `Settings.cs` and modify `LHOST` and `LPORT` to your preferred IP and port for the reverse shell.

2. **Build and Run**:
   To build the project, use:
    ```bash
    dotnet build
    ```
   To run the program, use:
    ```bash
    dotnet run
    ```

3. **Monitor the logs**:
   Logs are generated in the `GenEDRBypass.log` file for debugging and monitoring purposes.

## Example output
```
> dotnet run

[INFO] Генерация и обфускация шеллкода для обратного подключения к 192.168.1.100:4444...
[INFO] Запуск генерации шеллкода с использованием msfvenom...
[INFO] Шеллкод успешно сгенерирован. Выполняется парсинг...
[INFO] Парсинг завершен. Количество байтов в шеллкоде: 510
[INFO] Выполняется обфускация шеллкода для обхода сигнатур...
[INFO] Обфускация завершена.
[INFO] Проверка на наличие отладчика...
[INFO] Проверка на песочницу...
[INFO] Запутывание выполнения...
[INFO] Память выделена по адресу: 0x7ffdf000
[INFO] Защита памяти изменена на: PAGE_EXECUTE_READ
[INFO] Дешифруем и копируем шеллкод в выделенную память...
[INFO] Создание потока для инъекции шеллкода...
[INFO] Инъекция шеллкода прошла успешно.
[INFO] Память освобождена по адресу: 0x7ffdf000

[INFO] Выполнение завершено.
```

### Note
Make sure that you modify the shellcode to point to your own IP address and port for the reverse shell. The shellcode is dynamically generated for each run, ensuring that the signature changes, making detection by antivirus software more difficult.


## Technical Details
1. Uses `VirtualAlloc` to allocate memory with executable permissions.
2. Dynamically obfuscates shellcode to change its signature on each run.
3. Includes basic anti-debugging and anti-sandbox checks.
4. Creates a new thread to execute the payload in a stealthy manner.


## Disclaimer
This tool is intended for educational purposes and ethical hacking in controlled environments. Unauthorized use on systems you do not own is illegal. Use responsibly and only on systems you are permitted to test.



# RU
**GenEDRBypass** — это продвинутый инструмент для обхода систем обнаружения и реагирования на конечных точках (EDR) и антивирусного программного обеспечения. Он предназначен для профессионалов в области кибербезопасности и может использоваться для этичного хакерства, тестирования защиты и образовательных целей.


## Функционал
- **Динамическая генерация шеллкода**: Автоматически создает шеллкод с помощью `msfvenom`.
- **Техники обфускации**: Применяет шифрование XOR и динамическое изменение ключа для обхода сигнатурного обнаружения.
- **Управление памятью**: Выделяет и манипулирует памятью для выполнения шеллкода.
- **Антиотладка и антипесочница**: Реализует техники обхода отладчиков и песочниц.
- **Настраиваемые параметры**: IP-адрес и порт для обратного подключения легко настраиваются.

## Как начать работу
### Необходимое ПО
- .NET Core SDK
- `msfvenom` (часть фреймворка Metasploit)
- Администраторские права для полного функционала

### Установка
1. **Клонируйте репозиторий**:
    ```bash
    git clone https://github.com/geniuszly/GenEDRBypass
    cd GenEDRBypass
    ```

2. **Установите зависимости**:
    Убедитесь, что у вас установлен .NET Core. Вы можете скачать его [здесь](https://dotnet.microsoft.com/download).

### Использование
1. **Настройте `Settings.cs`**:
   Откройте `Settings.cs` и измените `LHOST` и `LPORT` на ваш IP и порт для обратного подключения.

2. **Сборка и запуск**:
   Для сборки проекта используйте:
    ```bash
    dotnet build
    ```
   Для запуска программы используйте:
    ```bash
    dotnet run
    ```

3. **Мониторинг логов**:
   Логи создаются в файле `GenEDRBypass.log` для отладки и мониторинга.

## Пример вывода
```
> dotnet run

[INFO] Генерация и обфускация шеллкода для обратного подключения к 192.168.1.100:4444...
[INFO] Запуск генерации шеллкода с использованием msfvenom...
[INFO] Шеллкод успешно сгенерирован. Выполняется парсинг...
[INFO] Парсинг завершен. Количество байтов в шеллкоде: 510
[INFO] Выполняется обфускация шеллкода для обхода сигнатур...
[INFO] Обфускация завершена.
[INFO] Проверка на наличие отладчика...
[INFO] Проверка на песочницу...
[INFO] Запутывание выполнения...
[INFO] Память выделена по адресу: 0x7ffdf000
[INFO] Защита памяти изменена на: PAGE_EXECUTE_READ
[INFO] Дешифруем и копируем шеллкод в выделенную память...
[INFO] Создание потока для инъекции шеллкода...
[INFO] Инъекция шеллкода прошла успешно.
[INFO] Память освобождена по адресу: 0x7ffdf000

[INFO] Выполнение завершено.
```

### Примечание
Не забудьте изменить шеллкод, чтобы он указывал на ваш собственный IP-адрес и порт для обратного подключения. Шеллкод генерируется динамически для каждого запуска, что обеспечивает изменение его сигнатуры и затрудняет обнаружение антивирусным ПО.


## Технические детали
1. Использует `VirtualAlloc` для выделения памяти с правами на выполнение.
2. Динамически обфусцирует шеллкод для изменения его сигнатуры при каждом запуске.
3. Включает базовые проверки на наличие отладчика и песочницы.
4. Создает новый поток для выполнения полезной нагрузки скрытным образом.

## Отказ от ответственности
Этот инструмент предназначен исключительно для образовательных целей и этичного хакерства в контролируемых условиях. Неавторизованное использование на системах, которые вам не принадлежат, является незаконным. Используйте ответственно и только на системах, на которых вы имеете право проводить тестирование.

