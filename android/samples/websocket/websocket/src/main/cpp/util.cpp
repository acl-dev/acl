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


