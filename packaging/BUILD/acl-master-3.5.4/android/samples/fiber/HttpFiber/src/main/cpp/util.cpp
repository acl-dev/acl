//
// Created by zhengshuxin on 2019/9/7.
//

#include "stdafx.h"
#include "util.h"

void util::JString2String(JNIEnv *env, jstring js, std::string &out)
{
    const char *ptr = env->GetStringUTFChars(js, 0);
    out = ptr;
    env->ReleaseStringUTFChars(js, ptr);
}

jstring util::String2JString(JNIEnv *env, const char *s)
{
    jstring js = env->NewStringUTF(s);
    return js;
}

//extern "C" int atexit(void (*)()) { return 0; }