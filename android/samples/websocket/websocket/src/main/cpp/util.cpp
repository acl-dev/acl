#include "stdafx.h"
#include "util.h"

void JString2String(JNIEnv *env, jstring js, std::string &out)
{
    const char *ptr = env->GetStringUTFChars(js, 0);
    out = ptr;
    env->ReleaseStringUTFChars(js, ptr);
}

jstring String2JString(JNIEnv *env, const char *s)
{
    jstring js = env->NewStringUTF(s);
    return js;
}

void log_info(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, "http-info", fmt, ap);
    va_end(ap);
}

void log_error(const char* fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_ERROR, "http-error", fmt, ap);
    va_end(ap);
}

static void write_callback(void*, const char* fmt, va_list ap)
{
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    __android_log_print(ANDROID_LOG_INFO, "dns-debug", "%s", buf);
}

static bool __opened = false;

static int open_callback(const char*, void*)
{
    return 0;
}

static void logger_hook(void (*write_callback)(void*, const char* fmt, va_list ap))
{
    if (__opened) {
        return;;
    }
    __opened = true;
    acl_msg_register(open_callback, NULL, write_callback, NULL);
    acl_msg_open("dummy.txt", "dummy");
    logger("acl lib init ok!");
}

static void logger_unhook(void)
{
    if (!__opened) {
        return;;
    }
    __opened = false;
    acl_msg_unregister();
}

void log_open(void)
{
    if (__opened) {
        return;
    }
    __opened = true;

    log_info("call logger_hook\r\n");
    logger_hook(write_callback);
    log_info("call logger_hook ok\r\n");
}

void log_close(void)
{
    if (__opened) {
        logger_unhook();
        __opened = false;
    }
}
