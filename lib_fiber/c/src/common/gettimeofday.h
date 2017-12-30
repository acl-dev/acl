#ifndef __GETTIMEOFDAY_HEAD_H__
#define __GETTIMEOFDAY_HEAD_H__

#include "define.h"

#ifdef SYS_WIN
int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

#endif
