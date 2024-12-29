#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include "cJSON.h"

int main(int argc, char *argv[]) {
    char buf[1024] = {};
    char *result = NULL;

    char pwd[PATH_MAX] = {};
    getcwd(pwd, PATH_MAX);

    cJSON *root = cJSON_CreateArray();
    while ((result = fgets(buf, sizeof(buf), stdin)) != NULL) {
        cJSON *args = cJSON_CreateArray();
        const char *arg = strtok(buf, " ");
        do {
            if (arg[0] == '\n') { break; }
            cJSON_AddItemToArray(args, cJSON_CreateString(arg));
        } while ((arg = strtok(NULL, " ")) != NULL);
        cJSON *source_file = cJSON_GetArrayItem(args, cJSON_GetArraySize(args) - 1);
        cJSON *target = cJSON_CreateObject();
        cJSON_AddItemToObject(target, "arguments", args);
        cJSON_AddItemToObject(target, "directory", cJSON_CreateString(pwd));
        cJSON_AddItemReferenceToObject(target, "file", source_file);
        cJSON_AddItemToArray(root, target);
    }

    puts(cJSON_Print(root));

    cJSON_Delete(root);

    return 0;
}
