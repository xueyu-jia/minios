void* memcpy(void* p_dst, void* p_src, int size){
	char* d = p_dst, *s = p_src;
	while(size > 0) {
		*(d++) = *(s++);
		size--;
	}
	return p_dst;
}

void memset(void* p_dst, char ch, int size){
	char* p = p_dst;
	while(size > 0) {
		*(p++) = ch;
		size--;
	}
}

char* strcat(char *dst, const char *src)
{
    if(dst==0 || src==0) return 0;
    char *temp = dst;
    while (*temp != '\0')
        temp++;
    while ((*temp++ = *src++) != '\0');

    return dst;
}

char* strcpy(char* p_dst, char* p_src){
	char c = 0, *p = p_dst;
	do{
		c = *(p_src++);
		*(p++) = c;
	}while(c);
	return p_dst;
}

int strlen(const char* p_str){
	int cnt = 0;
	while(*(p_str++)){
		cnt++;
	}
	return cnt;
}

//added by ran
char* strncpy(char * dest, const char *src, int n)
{
	if ((dest == 0) || (src == 0)) { /* for robustness */
		return dest;
	}
    int i;
    for (i = 0; i < n; i++)
    {
        dest[i] = src[i];
        if (!src[i])
        {
            break;
        }
    }
    return dest;
}
/*****************************************************************************
 *                                strcmp
 *****************************************************************************/
/**
 * Compare two strings.
 *
 * @param s1  The 1st string.
 * @param s2  The 2nd string.
 *
 * @return  an integer less than, equal to, or greater than zero if s1 (or the
 *          first n bytes thereof) is  found,  respectively,  to  be less than,
 *          to match, or be greater than s2.
 *****************************************************************************/
int strcmp(const char *s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0))
	{ /* for robustness */
		return (s1 - s2);
	}

	const char *p1 = s1;
	const char *p2 = s2;

	for (; *p1 && *p2; p1++, p2++)
	{
		if (*p1 != *p2)
		{
			break;
		}
	}

	return (*p1 - *p2);
}

//added by ran
int strncmp(const char *s1, const char *s2, int n)
{
    if (!s1 || !s2)
    {
        return s1 - s2;
    }
    int i;
    for (i = 0; i < n; i++)
    {
        if (s1[i] != s2[i])
        {
            return s1[i] - s2[i];
        }
    }
    return 0;
}

char *strchr(const char *str, char c) {
	char *p;
	for(p = str; *p; p++) {
		if(*p == c) {
			break;
		}
	}
	return p;
}