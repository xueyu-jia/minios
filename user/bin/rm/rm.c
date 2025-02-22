#include <malloc.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int remove(const char* path, bool recursive) {
    struct stat statbuf = {};
    const int retval = stat(path, &statbuf);
    if (retval != 0) { return -1; }
    switch (statbuf.st_type) {
        case I_DIRECTORY: {
            if (recursive) {
                char* pathbuf = malloc(MAX_PATH);
                strcpy(pathbuf, path);
                if (pathbuf[strlen(pathbuf) - 1] != '/') { strcat(pathbuf, "/"); }
                int len = strlen(pathbuf);
                DIR* dirp = opendir(path);
                struct dirent* sub;
                while ((sub = readdir(dirp)) != NULL) {
                    if ((strcmp(sub->d_name, ".") == 0) || (strcmp(sub->d_name, "..") == 0)) {
                        continue;
                    }
                    strcat(pathbuf, sub->d_name);
                    remove(pathbuf, recursive);
                    pathbuf[len] = 0;
                }
                closedir(dirp);
                free(pathbuf);
            }
            return rmdir(path);
        } break;
        case I_REGULAR: {
            return unlink(path);
        } break;
    }
    return -1;
}

int main(int argc, char* argv[]) {
    bool recursicve = false;
    char opt = '\0';
    while ((opt = getopt(argc, argv, "r")) != -1) {
        if (false) {
        } else if (opt == 'r') {
            recursicve = true;
        } else {
            break;
        }
    }
    if (optind < argc) { return remove(argv[optind], recursicve); }
    fprintf(stderr, "no target to remove\n");
    return -1;
}
