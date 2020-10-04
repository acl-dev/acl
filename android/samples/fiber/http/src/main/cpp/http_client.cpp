//
// Created by shuxin 　　zheng on 2020-03-29.
//

#include "stdafx.h"
#include "http_task.h"
#include "http_get.h"
#include "fiber_waiter.h"
#include "log.h"

static void JString2String(JNIEnv *env, jstring js, std::string &out)
{
    const char *ptr = env->GetStringUTFChars(js, 0);
    out = ptr;
    env->ReleaseStringUTFChars(js, ptr);
}

static jstring String2JString(JNIEnv *env, const char *s)
{
    jstring js = env->NewStringUTF(s);
    return js;
}

////////////////////////////////////////////////////////////////////////////////

static JavaVM* g_jvm   = NULL;
static jobject g_fiber = NULL;

static JNIEnv* get_env(void) {
    JNIEnv* env;
    int status = g_jvm->GetEnv((void**) &env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED || env == NULL) {
        log_info("get_env: status=%d, %d, env=%p",
                 status, JNI_EDETACHED, env);
        status = g_jvm->AttachCurrentThread(&env, NULL);
        if (status != JNI_OK || env == NULL) {
            log_error("get_env: AttachCurrentThread failed");
            return NULL;
        }
        log_info("get_env: status=%d, env=%p", status, env);
    }

    if (env->ExceptionOccurred()) {
        log_error("%d: some exception happened!", __LINE__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return NULL;
    }

    log_info("get_env: get env=%p ok, status=%d", env, status);
    return env;
}

static void test1(void) {
    log_info("test1: jvm=%p", g_jvm);

    // 先获得当前线程所绑定的 JNIEnv 对象
    JNIEnv* env = get_env();
    if (env == NULL) {
        log_error("test1: get_env null");
        return;
    }

    const char* name = "com/example/http/HttpFiberThread";
    jclass clazz = env->FindClass(name);
    log_info("test1: env=%p, clazz=%p", env, clazz);

    if (env->ExceptionOccurred()) {
        log_error("test1: %d: some exception happened!", __LINE__);
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

static void test2(void) {
    log_info("test2: jvm=%p", g_jvm);

    JNIEnv* env = get_env();
    if (env == NULL) {
        log_error("test2: get_env null");
        return;
    }
    jclass clazz = env->GetObjectClass(g_fiber);
    if (env->ExceptionOccurred()) {
        log_error("test2: %d: some exception happened!", __LINE__);
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    log_info("test2: env=%p, class=%p", env, clazz);
}

static void sleep_fiber(void) {
    while (true) {
        sleep(2);
        test2();
    }
}

static void fiber_thread_run(fiber_waiter* waiter) {
    waiter->start(256000);

    go[=] {
        sleep_fiber();
    };

    test1();
    log_info("---------------------------------------------------------------");
    test2();

    log_info(">>>>>>fiber schedule started<<<");
    acl::fiber::schedule();
    log_info(">>>>>>fiber schedule stopped<<<");
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_http_HttpFiberThread_FiberSchedule(
        JNIEnv* env,
        jobject me)
{
    log_open();

    // 设置 JVM 全局变量
    env->GetJavaVM(&g_jvm);
    g_fiber = env->NewGlobalRef(me);

    log_info("FiberSchedule: first g_jvm=%p", g_jvm);

    // create waiter fiber to wait for http task
    fiber_waiter* waiter = new fiber_waiter;
    test1();
    log_info("---------------------------------------------------------------");

    // create one thread to schedule fibers process
    std::thread* thread = new std::thread(fiber_thread_run, waiter);
    thread->detach();
    return (jlong) waiter;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_http_HttpFiberThread_HttpGet(
        JNIEnv* env,
        jobject me,
        jlong o,
        jstring host,
        jint port,
        jstring url)
{
    fiber_waiter* waiter = (fiber_waiter*) o;

    std::string server_host, server_url;
    JString2String(env, host, server_host);
    JString2String(env, url, server_url);

    log_info("%s(%d), HttpGet: host=%s, port=%d, url=%s", __FILE__, __LINE__,
            server_host.c_str(), port, server_url.c_str());

    http_task t(env, server_host.c_str(), port, server_url.c_str());

    // add one http request to waiting queue
    waiter->push(&t);

    // waiting for the http result
    t.wait();

    const acl::string& s = t.body();
    if (s.empty()) {
        return NULL;
    }

    return String2JString(env, s.c_str());
}
