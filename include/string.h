#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void memset(void *s, int c, size_t n);
int strcmp(const char *s1, const char *s2);
char* itoa(int num);
int strlen(const char *s);
void *strcpy(char *dest, const char *src);

#endif