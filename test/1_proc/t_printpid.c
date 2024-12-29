#include <stdio.h>

int global = 1; // added by mingxuan 2020-12-21

int main(int arg, char *argv[]) {
    printf("%d", get_pid());
    return 0;
}
