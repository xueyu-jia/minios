#include <stdio.h>
#include <syscall.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: umount %%mountpoint\n");
        return -1;
    }
    return umount(argv[1]);
}
