//
// Created by shuxin 　　zheng on 2020/10/4.
//

#include "stdafx.h"
#include "log.h"
#include "fiber_waiter.h"
#include "jobject_set.h"

static JavaVM* g_jvm = NULL;

static JNIEnv* get_env(void) {
    JNIEnv* env;
    int status = g_jvm->GetEnv((void**) &env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED || env == NULL) {
        log_info("get_env(%d): status=%d, %d, env=%p",
                 __LINE__, status, JNI_EDETACHED, env);

        log_info("try call AttachCurrentThread to get env, jvm=%p", g_jvm);
        //status = g_jvm->AttachCurrentThreadAsDaemon(&env, NULL);
        status = g_jvm->AttachCurrentThread(&env, NULL);
        if (status != JNI_OK || env == NULL) {
            log_error("get_env: AttachCurrentThread failed");
            return NULL;
        }
        log_info("get_env(%d): status=%d, env=%p", __LINE__, status, env);
    }

    if (env->ExceptionOccurred()) {
        log_error("%d: some exception happened!", __LINE__);
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    log_info("get_env: get env=%p ok, status=%d", env, status);
    return env;
}

static void fiber_thread_run(fiber_waiter* waiter) {
    waiter->start(1024000);
    JNIEnv* env = get_env();
    log_info(">>>get env=%p<<<", env);
    acl::fiber::schedule();
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_HttpFiber_FiberThread_FiberSchedule(
        JNIEnv* env,
        jobject me)
{
    env->GetJavaVM(&g_jvm);
    fiber_waiter* waiter = new fiber_waiter;
    std::thread*  thread = new std::thread(fiber_thread_run, waiter);
    thread->detach();
    return (jlong) waiter;
}

class myfiber : public acl::fiber {
public:
    myfiber(jobject o) : o_(o) {
    }

private:
    ~myfiber(void) {
        JNIEnv* env = get_env();
        if (env) {
            env->DeleteGlobalRef(o_);
        }
    }

public:
    void run(void) {
        log_info(">>>fiber started<<<");
        JNIEnv* env = get_env();
        if (env == NULL) {
            return;
        }

        log_info(">>>run: env=%p, o=%p<<<", env, o_);
        //jclass clz = env->GetObjectClass(o_);
        //log_info(">>>o=%p, clz=%p<<", o_, clz);

        jobject fb = env->NewGlobalRef(o_);
        jobject_set oper(env, fb);
        oper.call("onRun");
        //delete this;
    }

private:
    jobject o_;
};

static void thread_call(myfiber* fb) {
    log_info("-----------begin-----------------");
    fb->run();
    log_info("-----------end-------------------");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_HttpFiber_FiberThread_FiberStart(
        JNIEnv *env,
        jobject thiz,
        jlong owaiter,
        jobject ofiber)
{
    jobject fb = env->NewGlobalRef(ofiber);

    fiber_waiter* waiter = (fiber_waiter*) owaiter;
    myfiber* fiber = new myfiber(fb);
    log_info("start one fiber");

    if (0) {
        std::thread thread(thread_call, fiber);
        thread.detach();
    } else {
        waiter->push(fiber);
    }
}
