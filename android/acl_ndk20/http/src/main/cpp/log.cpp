#include "stdafx.h"
#include "log.h"

void log_info(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, "dns-info", fmt, ap);
    va_end(ap);
}

void log_error(const char* fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_ERROR, "dns-error", fmt, ap);
    va_end(ap);
}
