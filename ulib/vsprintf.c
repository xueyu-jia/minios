#include <stdio.h>
#include <string.h>
#include <fmt.h>
#include <limits.h>
#include <stdarg.h>
#include <ctype.h>

int vsprintf(char *buf, const char *fmt, va_list args) {
    return vnstrfmt(buf, SSIZE_MAX, fmt, args);
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list arg = (va_list)((char *)(&fmt) + 4);
    return vsprintf(buf, fmt, arg);
}

char getchar() {
    char ch;
    return read(STD_IN, &ch, 1) == 1 ? ch : EOF;
}

char fgetc(int fd) {
    char ch;
    return read(fd, &ch, 1) == 1 ? ch : EOF;
}

char *fgets(char *str, int size, int fd) {
    char c;
    char *cs;
    cs = str;
    while (size && (c = fgetc(fd)) != EOF) {
        if ((*cs = c) == '\n') { break; }
        cs++;
        size--;
    }
    *cs = '\0';
    return str;
}

char *gets(char *str) {
    char c;
    char *cs;
    cs = str;
    while ((c = getchar()) != EOF) {
        if ((*cs = c) == '\n') { break; }
        cs++;
    }
    *cs = '\0';
    return str;
}

unsigned long strtoul(const char *cp, char **endp, unsigned int base) {
    unsigned long result = 0, value;
    if (!base) {
        base = 10;
        if (*cp == '0') {
            base = 8;
            cp++;
            if ((tolower(*cp) == 'x') && isxdigit(cp[1])) {
                cp++;
                base = 16;
            }
        }
    } else if (base == 16) {
        if (cp[0] == '0' && tolower(cp[1]) == 'x') cp += 2;
    }
    while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp - '0' : tolower(*cp) - 'a' + 10) < base) {
        result = result * base + value;
        cp++;
    }
    if (endp) *endp = (char *)cp;
    return result;
}

long strtol(const char *cp, char **endp, unsigned int base) {
    if (*cp == '-') return -strtoul(cp + 1, endp, base);
    return strtoul(cp, endp, base);
}

int atoi(const char *nptr) {
    return strtol(nptr, (char **)NULL, 10);
}
