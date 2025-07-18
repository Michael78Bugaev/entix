#include <string.h>
#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n)
{
    char *d = dest;
    const char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

void memset(void *s, int c, size_t n)
{
    char *p = s;
    while (n--)
        *p++ = c;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

char* itoa(int num) {
    static char buf[12]; //Достаточно для int32 с учетом знака и \0
    int i = 10;
    int is_negative = 0;
    buf[11] = '\0';

    if (num == 0) {
        buf[10] = '0';
        return &buf[10];
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num && i) {
        buf[i--] = (num % 10) + '0';
        num /= 10;
    }

    if (is_negative) {
        buf[i--] = '-';
    }

    return &buf[i + 1];
}

int strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

void *strcpy(char *dest, const char *src) {
    char *p = dest;
    while (*src) *p++ = *src++;
    *p = '\0';
    return dest;
}