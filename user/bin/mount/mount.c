#include <stdio.h>
#include <stddef.h>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("usage: mount %%dev %%target [ %%fstype ]\n");
        return -1;
    }
    return mount(argv[1], argv[2], argc == 3 ? NULL : argv[3], 0, NULL);
}
