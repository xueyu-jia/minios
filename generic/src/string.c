#include <string.h>
#include <ctype.h>

void *memcpy(void *p_dst, const void *p_src, size_t size) {
    char *d = p_dst;
    const char *s = p_src;
    while (size > 0) {
        *(d++) = *(s++);
        size--;
    }
    return p_dst;
}

void memset(void *p_dst, char ch, size_t size) {
    char *p = p_dst;
    while (size > 0) {
        *(p++) = ch;
        size--;
    }
}

int memcmp(const void *p1, const void *p2, int size) {
    const char *pchar1 = p1, *pchar2 = p2;
    char c1, c2;
    while (size > 0) {
        c1 = *(pchar1++);
        c2 = *(pchar2++);
        if (c1 != c2) { return c1 - c2; }
        size--;
    }
    return 0;
}

char *strcat(char *dst, const char *src) {
    if (dst == 0 || src == 0) return 0;
    char *temp = dst;
    while (*temp != '\0') temp++;
    while ((*temp++ = *src++) != '\0');

    return dst;
}

char *strcpy(char *p_dst, const char *p_src) {
    char c = 0, *p = p_dst;
    do {
        c = *(p_src++);
        *(p++) = c;
    } while (c);
    return p_dst;
}

int strlen(const char *p_str) {
    int cnt = 0;
    while (*(p_str++)) { cnt++; }
    return cnt;
}

char *strncpy(char *dest, const char *src, int n) {
    if ((dest == 0) || (src == 0)) { return dest; }
    int i;
    for (i = 0; i < n; ++i) {
        dest[i] = src[i];
        if (!src[i]) { break; }
    }
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    if ((s1 == 0) || (s2 == 0)) { return (s1 - s2); }

    const char *p1 = s1;
    const char *p2 = s2;

    for (; *p1 && *p2; p1++, p2++) {
        if (*p1 != *p2) { break; }
    }

    return (*p1 - *p2);
}

int strncmp(const char *s1, const char *s2, int n) {
    if (!s1 || !s2) { return s1 - s2; }
    int i;
    for (i = 0; i < n; ++i) {
        if (s1[i] != s2[i]) { return s1[i] - s2[i]; }
    }
    return 0;
}

int stricmp(const char *s1, const char *s2) {
    unsigned char c1, c2;

    c1 = 0;
    c2 = 0;
    if (!s1 || !s2) { return s1 - s2; }
    for (; (c1 = *s1, c2 = *s2, c1 && c2); s1++, s2++) {
        if (c1 == c2) continue;
        c1 = tolower(c1);
        c2 = tolower(c2);
        if (c1 != c2) break;
    }
    return (int)c1 - (int)c2;
}

char *strchr(const char *s, char c) {
    char ch = (char)c;
    while (*s != '\0') {
        if (*s == ch) { return (char *)s; }
        s++;
    }
    return NULL;
}

char *strrchr(const char *s, char c) {
    char ch = (char)c;
    char *last_occurrence = NULL;

    for (const char *p = s + strlen(s) - 1; p >= s; p--) {
        if (*p == ch) { last_occurrence = (char *)p; }
    }

    return last_occurrence;
}

char *itoa(int num, char *str, int radix) {
    char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned unum;
    int i = 0, j, k = 0;
    if (radix == 10 && num < 0) {
        unum = (unsigned)-num;
        str[i++] = '-';
        k = 1;
    } else
        unum = (unsigned)num;
    do {
        str[i++] = index[unum % (unsigned)radix];
        unum /= radix;
    } while (unum);
    str[i] = '\0';
    char temp;
    for (j = k; j <= (i - 1) / 2; ++j) {
        temp = str[j];
        str[j] = str[i - 1 + k - j];
        str[i - 1 + k - j] = temp;
    }
    return str;
}

char *strstr(const char *s1, const char *s2) {
    const char *p = s1;
    const size_t len = strlen(s2);

    if (!len) { return (char *)s1; }

    for (; (p = strchr(p, *s2)) != 0; ++p) {
        if (strncmp(p, s2, len) == 0) { return (char *)p; }
    }
    return NULL;
}
