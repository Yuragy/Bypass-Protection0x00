<img src="https://raw.githubusercontent.com/D3Ext/Nimbus/main/images/logo.webp" alt="logo">

# Nimbus

Shellcode loader with evasion capabilities written in Nim

## Features

- Inject AES encrypted shellcode
- Direct syscalls by retrieving STUBS during runtime
- ntdll.dll unhooking
- Basic anti-sandbox checks
- AMSI and ETW patching
- Custom sleep function

## Usage

This loader makes use of the AES encryption algorithm so in order to make it work, you need to encrypt your own shellcode. To do so you may use either `aes_encrypt.py` or `aes_encrypt.nim`. Both scripts will generate a random PSK and IV and it will take care of encrypting your shellcode so that you only have to modify the variables at the very top of the file.

Encrypt your raw shellcode:

```sh
$ nim r aes_encrypt.nim calc.bin
```

or 

```sh
$ python3 aes_encrypt.py calc.bin
```

Once you have modified `nimbus.nim` to suit your needs, you just have to compile it like this:

```sh
$ nim c -d=mingw -d:release --cpu=amd64 nimbus.nim
```

Or simply using `make`:

```sh
$ make
```

## Installation

You need to have installed `nim` and some specific packages:

```
$ nimble install winim nimcrypto psutil ptr_math
```

## Demo

For testing purposes I have used a simple `calc.exe` shellcode. You can generate it using `msfvenom` like this:

```sh
$ msfvenom -p windows/x64/exec CMD="calc.exe" -f raw -o calc.bin
```

Tested on x64

<img src="https://raw.githubusercontent.com/D3Ext/Nimbus/main/images/compile.png" alt="compile">

<img src="https://raw.githubusercontent.com/D3Ext/Nimbus/main/images/demo.png" alt="demo">

As can be seen, the shellcode gets decrypted and injected successfully

If I upload the EXE to [KleenScan](https://www.kleenscan.com/index) (an alternative to VirusTotal that promises not to distribute the malware) we see that it seems legit with 0 detections

<img src="https://raw.githubusercontent.com/D3Ext/Nimbus/main/images/scan.png" alt="scan">

## References

```
https://github.com/byt3bl33d3r/OffensiveNim
https://github.com/S3cur3Th1sSh1t/NimGetSyscallStub
https://github.com/itaymigdal/PartyLoader
https://github.com/RistBS/Awesome-RedTeam-Cheatsheet
https://github.com/S3cur3Th1sSh1t/Nim-RunPE
https://github.com/icyguider/Nimcrypt2
https://github.com/chvancooten/maldev-for-dummies
https://redops.at/en/blog/syscalls-via-vectored-exception-handling
```


