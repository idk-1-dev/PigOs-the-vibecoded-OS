#pragma once
// pigOS libc stubs for lwIP
// These provide the minimal C library function declarations used by lwIP/pigOS.
#include <stdint.h>
#include <stddef.h>

void* pig_memcpy(void* dst, const void* src, unsigned long n);
void* pig_memmove(void* dst, const void* src, unsigned long n);
void* pig_memset(void* s, int c, unsigned long n);
int pig_memcmp(const void* s1, const void* s2, unsigned long n);
unsigned long pig_strlen(const char* s);
int pig_strncmp(const char* s1, const char* s2, unsigned long n);
int pig_strcmp(const char* s1, const char* s2);
char* pig_strcpy(char* dst, const char* src);
long pig_strtol(const char* nptr, char** endptr, int base);
uint32_t sys_now(void);

// Standard libc-like symbols that lwIP/object files may reference directly.
void* memcpy(void* dst, const void* src, size_t n);
void* memmove(void* dst, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
size_t strlen(const char* s);
int strncmp(const char* s1, const char* s2, size_t n);
int strcmp(const char* s1, const char* s2);
char* strcpy(char* dst, const char* src);
long strtol(const char* nptr, char** endptr, int base);

const unsigned short** __ctype_b_loc(void);
const int** __ctype_tolower_loc(void);
