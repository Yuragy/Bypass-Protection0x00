#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

#ifndef XOR_KEY
#define XOR_KEY 0xff
#endif

int main(int argc, char **argv)
{
    int payload_fd;
    int enc_payload_fd;
    unsigned char *payload_map;
    unsigned char *enc_payload_map;
    struct stat statbuf;

    payload_fd = open(argv[1], O_RDONLY);
    fstat(payload_fd, &statbuf);
    enc_payload_fd = open(argv[2], O_RDWR | O_CREAT, S_IRWXU);
    ftruncate(enc_payload_fd, statbuf.st_size);
    payload_map = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, payload_fd, 0);
    enc_payload_map = mmap(NULL, statbuf.st_size, PROT_WRITE, MAP_SHARED, enc_payload_fd, 0);

    for (int i = 0; i < statbuf.st_size; i++) {
        enc_payload_map[i] = payload_map[(statbuf.st_size-1) - i] ^ XOR_KEY;
    }

    munmap(payload_map, statbuf.st_size);
    munmap(enc_payload_map, statbuf.st_size);
    close(payload_fd);
    close(enc_payload_fd);

    return 0;
}
