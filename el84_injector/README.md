# el84_injector_0.0

`el84_injector_0.0` is a code injection tool developed for educational and security research purposes. It demonstrates advanced techniques in code injection and evasion, focusing on Windows environments. The tool utilizes API hashing, custom encoding/decoding methods, and direct manipulation of process memory to inject payloads into target processes without detection by Windows Defender.

![image](https://github.com/rat-c/el84_injector_0.0/assets/89196953/547e42a1-4090-47c5-910a-02713f6477b5)

## Features

- **API Hashing**: Utilizes API hashing to locate necessary WinAPI functions dynamically.
- **XOR Encoding**: Encodes WinAPI function pointers with a random 64-bit value (XOR) to evade static analysis.
- **FCALL Macro**: Simplifies WinAPI calls, enhancing code readability and maintainability.
- **Custom Encoding/Decoding**: Implements a basic custom method for payload encoding and decoding to bypass security measures.
- **Simple Usage**: Designed for easy use, requiring only the target process ID and a payload saved as `shellcode.bin` in the project directory.

## Usage
1. Place your x64 payload in a file named `shellcode.bin` in the project directory.
2. Run `build.sh` to compile the injector. This script automates the process, including payload encoding and injector compilation.
3. Execute `injector.exe` with the target process ID as the command line argument:

```bash
./injector.exe <pid>
```

## Building from Source
The project can be built on systems with GCC and Mingw-w64 installed. The build process involves compiling helper programs for hash generation and payload encoding, followed by the main injector program.

**Dependencies:**
- GCC (for hash and encode utilities)
- x86_64-w64-mingw32-gcc (for compiling the injector on non-Windows platforms)
- Any necessary libraries for Windows API calls

**Build Steps:**
Refer to the `build.sh` script for detailed build commands and steps.

## Disclaimer
This tool is intended for educational and security research purposes only. The author is not responsible for misuse or for any damage that may occur from using this tool. It is the end user's responsibility to comply with all applicable laws and regulations. The use of this tool against targets without prior mutual consent is illegal.

## References
- API Hashing and evasion techniques
- Custom payload encoding/decoding methods
- [INCBIN GitHub Repository](https://github.com/graphitemaster/incbin) for binary inclusion in C/C++
- [64-Bit Programming Models: Why LP64?](https://unix.org/version2/whatsnew/lp64_wp.html)
