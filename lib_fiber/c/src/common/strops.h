#ifndef __STROPS_HEADER_H__
#define __STROPS_HEADER_H__

#include "define.h"

#ifndef SAFE_STRNCPY
#define SAFE_STRNCPY(_obj, _src, _size) do {                \
    if (_size > 0) {                                            \
        size_t _n = strlen(_src);                               \
        _n = _n > (size_t ) _size - 1? (size_t) _size - 1 : _n; \
        memcpy(_obj, _src, _n);                                 \
        _obj[_n] = 0;                                           \
    }                                                           \
} while (0)
#endif

#ifdef SYS_WIN
int strncasecmp(const char *s1, const char *s2, size_t n);
#endif

char *mystrtok(char **src, const char *sep);
char *lowercase(char *s);
int   alldig(const char *s);

#endif
