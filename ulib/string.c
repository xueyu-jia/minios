#include <string.h>
#include <malloc.h>

char *strdup(const char *s) {
    if (s == NULL) { return NULL; }
    const size_t len = strlen(s);
    char *cloned = malloc(len + 1);
    if (cloned == NULL) { return NULL; }
    strcpy(cloned, s);
    return cloned;
}
