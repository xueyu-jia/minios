#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("usage: unlink %%path\n");
        return -1;
    }
    return unlink(argv[1]);
}
