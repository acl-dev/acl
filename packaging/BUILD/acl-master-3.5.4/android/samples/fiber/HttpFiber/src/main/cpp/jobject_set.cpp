#include "stdafx.h"
#include "log.h"
#include "jobject_set.h"
#include "util.h"

jobject_set::jobject_set(JNIEnv *env, jobject obj)
: env_(env)
, obj_(obj)
{
    clz_ = env_->GetObjectClass(obj_);
    if (clz_ == NULL) {
        log_error("GetObjectClass error");
        abort();
    }

#if 1
    jmethodID foo = env_->GetMethodID(clz_, "<init>", "()V");
    jobject o = env_->NewObject(clz_, foo, "()V");
    log_info(">>>new obj=%p, clz=%p, env=%p<<<", o, clz_, env_);

    if (env_->ExceptionOccurred()) {
        log_error("%d: some exception happened!", __LINE__);
        env_->ExceptionDescribe();
        env_->ExceptionClear();
        //abort();
    }
#endif
}

bool jobject_set::set(const char *name, int in)
{
    jfieldID fID = env_->GetFieldID(clz_, name, "I");
    if (fID == NULL) {
        log_error("%s not found", name);
        return false;
    }
    env_->SetIntField(obj_, fID, (jint) in);
    return true;
}

bool jobject_set::set(const char *name, long long int in)
{
    jfieldID fID = env_->GetFieldID(clz_, name, "I");
    if (fID == NULL) {
        log_error("%s not found", name);
        return false;
    }
    env_->SetIntField(obj_, fID, (jlong) in);
    return true;
}

bool jobject_set::set(const char *name, const std::string &in)
{
    jfieldID fID = env_->GetFieldID(clz_, name, "Ljava/lang/String;");
    if (fID == NULL) {
        log_error("%s not found", name);
        return false;
    }
    jstring str = util::String2JString(env_, in.c_str());
    if (str == NULL) {
        log_error("GetString error, field=%s", name);
        return false;
    }

    env_->SetObjectField(obj_, fID, str);
    env_->DeleteLocalRef(str);
    return true;
}

bool jobject_set::set(const char *name, const std::vector<const char*>& in)
{
    jfieldID fID = env_->GetFieldID(clz_, name, "[Ljava/lang/String;");
    if (fID == NULL) {
        log_error("%s not found", name);
        return false;
    }
    jclass clazz = env_->FindClass("java/lang/String");
    if (clazz == NULL) {
        log_error("not found java/lang/String");
        return false;
    }

    jobjectArray array = env_->NewObjectArray(in.size(), clazz, NULL);
    env_->DeleteLocalRef(clazz);

    int i = 0;
    for (std::vector<const char*>::const_iterator cit = in.begin();
        cit != in.end(); ++cit, ++i) {
        jstring js = util::String2JString(env_, (*cit));
        env_->SetObjectArrayElement(array, i, js);
        env_->DeleteLocalRef(js);
    }
    env_->SetObjectField(obj_, fID, array);
    return true;
}

bool jobject_set::set(const char *name, const std::vector<std::string> &in)
{
    jfieldID fID = env_->GetFieldID(clz_, name, "[Ljava/lang/String;");
    if (fID == NULL) {
        log_error("%s not found", name);
        return false;
    }
    jclass clazz = env_->FindClass("java/lang/String");
    if (clazz == NULL) {
        log_error("not found java/lang/String");
        return false;
    }

    jobjectArray array = env_->NewObjectArray(in.size(), clazz, NULL);
    env_->DeleteLocalRef(clazz);

    int i = 0;
    for (std::vector<std::string>::const_iterator cit = in.begin();
         cit != in.end(); ++cit, ++i) {
        jstring js = util::String2JString(env_, (*cit).c_str());
        env_->SetObjectArrayElement(array, i, js);
        env_->DeleteLocalRef(js);
    }
    env_->SetObjectField(obj_, fID, array);
    return true;
}

bool jobject_set::call(const char* method)
{
    jmethodID mID = env_->GetMethodID(clz_, method, "()V");
    //jmethodID mID = env_->GetStaticMethodID(clz_, method, "()V");
    if (!mID) {
        log_error("method %s not found", method);
        return false;
    }

    if (env_->ExceptionOccurred()) {
        log_error("%d: some exception happened!", __LINE__);
        env_->ExceptionDescribe();
        env_->ExceptionClear();
    }

    log_info(">>>call method=%s, env=%p, clz=%p, obj=%p, mid=%p<<<", method, &env_, clz_, obj_, mID);
    env_->CallVoidMethod(obj_, mID);
    //env_->CallStaticVoidMethod(clz_, mID);
    log_info(">>>call finished<<<");

    if (env_->ExceptionOccurred()) {
        log_error("%d: some exception happened!", __LINE__);
        env_->ExceptionDescribe();
        env_->ExceptionClear();
    }
    return true;
}

bool jobject_set::call(const char *method, const std::vector<const char*>& in)
{
    jmethodID mID = env_->GetMethodID(clz_, method, "([Ljava/lang/String;)V");
    if (!mID) {
        log_error("method %s not found", method);
        return false;
    }
    jclass clazz = env_->FindClass("java/lang/String");
    if (clazz == NULL) {
        log_error("not found java/lang/String");
        return false;
    }

    jobjectArray array = env_->NewObjectArray(in.size(), clazz, NULL);
    env_->DeleteLocalRef(clazz);

    int i = 0;
    for (std::vector<const char*>::const_iterator cit = in.begin();
         cit != in.end(); ++cit, ++i) {
        jstring js = util::String2JString(env_, (*cit));
        env_->SetObjectArrayElement(array, i, js);
        env_->DeleteLocalRef(js);
    }
    env_->CallVoidMethod(obj_, mID, array);
    return true;
}

bool jobject_set::call(const char *method, const std::vector<std::string> &in)
{
    jmethodID mID = env_->GetMethodID(clz_, method, "([Ljava/lang/String;)V");
    if (!mID) {
        log_error("method %s not found", method);
        return false;
    }
    jclass clazz = env_->FindClass("java/lang/String");
    if (clazz == NULL) {
        log_error("not found java/lang/String");
        return false;
    }

    jobjectArray array = env_->NewObjectArray(in.size(), clazz, NULL);
    env_->DeleteLocalRef(clazz);

    int i = 0;
    for (std::vector<std::string>::const_iterator cit = in.begin();
         cit != in.end(); ++cit, ++i) {
        jstring js = util::String2JString(env_, (*cit).c_str());
        env_->SetObjectArrayElement(array, i, js);
        env_->DeleteLocalRef(js);
    }
    env_->CallVoidMethod(obj_, mID, array);
    return true;
}

