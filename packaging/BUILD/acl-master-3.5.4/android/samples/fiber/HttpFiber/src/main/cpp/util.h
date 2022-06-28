//
// Created by zhengshuxin on 2019/9/7.
//

#pragma once
#include <jni.h>

class util {
public:
    util(void) {}
    ~util(void) {}

    static void JString2String(JNIEnv* env, jstring js, std::string& out);
    static jstring String2JString(JNIEnv* env, const char* s);
};
