#include <env.h>
// each process's env only affect it self and child proc.
// this lib just manipulate current process
char **ENVP_LIN = (char **)0xBFFFF008;

char *getenv(const char *key) {
    char *split;
    for (char *pstr = *ENVP_LIN; pstr; pstr++) {
        split = strchr(pstr, '=');
        if (split != 0) {
            if (strncmp(key, pstr, split - pstr) == 0) { return split + 1; }
        }
    }
    return 0;
}
