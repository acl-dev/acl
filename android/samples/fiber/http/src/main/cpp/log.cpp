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

typedef void (*write_fn)(void*, const char*, va_list);

static int open_callback(const char*, void*)
{
    return 0;
}

static void write_callback(void*, const char* fmt, va_list ap)
{
    char tag[256];
    snprintf(tag, sizeof(tag), "debug");

    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);

    __android_log_print(ANDROID_LOG_INFO, tag, "%s", buf);
}

static void fiber_logger_callback(void *ctx, const char *fmt, va_list ap)
{
    __android_log_vprint(ANDROID_LOG_ERROR, "fiber", fmt, ap);
}

void log_open(void)
{
    acl_msg_register(open_callback, NULL, write_callback, NULL);
    acl_msg_open("dummy.txt", "dummy");
    logger("acl lib init ok!");

    acl_fiber_msg_register(fiber_logger_callback, NULL);
}
