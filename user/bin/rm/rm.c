#include <malloc.h>
#include <opt.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>

struct stat statbuf;
int remove(const char* path, int rec) {
    if (stat(path, &statbuf) == 0) {
        if (statbuf.st_type == I_REGULAR) {
            return unlink(path);
        } else if (statbuf.st_type == I_DIRECTORY) {
            if (rec == 1) {
                char* _path = (char*)malloc(MAX_PATH);
                strcpy(_path, path);
                if (_path[strlen(_path) - 1] != '/') { strcat(_path, "/"); }
                int len = strlen(_path);
                DIR* dirp = opendir(path);
                struct dirent* sub;
                while ((sub = readdir(dirp)) != NULL) {
                    if ((strcmp(sub->d_name, ".") == 0) || (strcmp(sub->d_name, "..") == 0)) {
                        continue;
                    }
                    strcat(_path, sub->d_name);
                    // printf("%s %d ", _path, rec);
                    remove(_path, rec);
                    _path[len] = 0;
                }
                closedir(dirp);
                free(_path);
            }
            return rmdir(path);
        }
    }
    return -1;
}

int main(int argc, char** argv) {
    int recursicve = 0, opt;
    while ((opt = getopt(argc, argv, "r")) != -1) {
        switch (opt) {
            case 'r':
                recursicve = 1;
                break;

            default:
                break;
        }
    }
    if (optind < argc) {
        return remove(argv[optind], recursicve);
    } else {
        fprintf(STD_ERR, "no target to remove\n");
        return -1;
    }
}
