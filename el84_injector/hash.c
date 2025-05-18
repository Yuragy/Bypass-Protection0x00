#include <stdint.h>
#include <stdio.h>

// djb2 from http://www.cse.yorku.ca/~oz/hash.html
uint64_t hash(unsigned char* str)
{
    uint64_t hash = 5381;
    unsigned char c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

struct ftable {
    char* mod;
    char* fname;
} table[] = {
    {"KERNEL32.dll", "OpenProcess"},
    {"KERNEL32.dll", "VirtualAllocEx"},
    {"KERNEL32.dll", "WriteProcessMemory"},
    {"KERNEL32.dll", "CreateRemoteThread"}
};

int main(int argc, char** argv)
{
    for (int i = 0; i < sizeof(table) / sizeof(struct ftable); i++) {
        printf("#define %s_HASH\t\t0x%llx\n",
            table[i].fname, hash(table[i].mod) + hash(table[i].fname) );
    }
}
