#include <stdio.h>
#include <stdbool.h>

int main(int argc, char* argv[]) {
#define MAX_CAT 128
    char buf[MAX_CAT];
    if (argc != 2) {
        printf("usage: cat %%path\n");
        return -1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("open error");
        return -1;
    }
    bool failed = false;
    do {
        int cnt = 0;
        do {
            cnt = read(fd, buf, MAX_CAT);
            if (cnt < 0) {
                failed = true;
                break;
            }
            write(STD_OUT, buf, cnt);
        } while (cnt == MAX_CAT);
        if (failed) {
            //! TODO: might perm denied rather than is-dir
            fprintf(stderr, "cat: %s: is a directory\n", argv[1]);
            break;
        }
        printf("\n");
    } while (0);
    close(fd);
    return 0;
}
