//ported by sundong 2023.3.26
#include <mbr/string.h>
int strlen(const char *s)
{
	int n = 0;

	for (n = 0; *s != '\0'; s++)
		n++;
	return n;
}

int strncmp(const char *p, const char *q, size_t n)
{
	while (n > 0 && *p && *p == *q)
		n--, p++, q++;
	if (n == 0)
		return 0;
	else
		return (int) ((unsigned char) *p - (unsigned char) *q);
}

void * memset(void *v, int c, size_t n)
{
	char *p;
	int m;

	p = v;
	m = n;
	while (--m >= 0)
		*p++ = c;

	return v;
}

void * memcpy(void *dst, const void *src, size_t n)
{
	const char *s;
	char *d;

	s = src;
	d = dst;

	if (s < d && s + n > d) {
		s += n;
		d += n;
		while (n-- > 0)
			*--d = *--s;
	} else {
		while (n-- > 0)
			*d++ = *s++;
	}

	return dst;
}
int strcmp(const char *p, const char *q)
{
	while (*p && *p == *q)
		p++, q++;
	return (int) ((unsigned char) *p - (unsigned char) *q);
}
