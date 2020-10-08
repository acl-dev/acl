//
// Created by zhengshuxin on 2019/9/10.
//

#pragma once
#include <jni.h>
#include <string>
#include <vector>

class jobject_set {
public:
    jobject_set(JNIEnv* env, jobject obj);
    ~jobject_set() {}

    bool set(const char* name, int in);
    bool set(const char* name, long long int in);
    bool set(const char* name, const std::string& in);
    bool set(const char* name, const std::vector<const char*>& in);
    bool set(const char* name, const std::vector<std::string>& in);

    bool call(const char* method);
    bool call(const char* method, const std::vector<const char*>& in);
    bool call(const char* method, const std::vector<std::string>& in);

private:
    JNIEnv* env_;
    jobject obj_;
    jclass  clz_;
};

