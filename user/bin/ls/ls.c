#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>

int main(int argc, char* argv[]) {
    char* path = "."; // default
    if (argc > 2) {
        printf("usage: ls [%%path]\n");
        return -1;
    } else if (argc == 2) {
        path = argv[1];
    }
    DIR* dirp = opendir(path);
    if (!dirp) {
        printf("opendir err\n");
        return -1;
    }
    struct dirent* ent = NULL;
    while ((ent = readdir(dirp)) != NULL) { printf("%d %s\n", ent->d_ino, ent->d_name); }
    closedir(dirp);
    return 0;
}
