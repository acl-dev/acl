//
// Created by shuxin 　　zheng on 2020-03-29.
//

#include "stdafx.h"
#include "util.h"
#include "websocket.h"

static void websocket_callback(void* env_ctx, void* obj_ctx, const char* msg)
{
    JNIEnv* env = (JNIEnv*) env_ctx;
    jobject obj = (jobject) obj_ctx;

    jclass clz = env->GetObjectClass(obj);
    if (clz) {
        jmethodID mID = env->GetMethodID(clz, "onMessage", "(Ljava/lang/String;)V");
        if (mID) {
            jstring s = String2JString(env, msg);
            env->CallVoidMethod(obj, mID, s);
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_http_HttpClient_WebsocketStart(
        JNIEnv* env,
        jobject obj,
        jstring addr)
{
    log_open();
    log_info("START!\r\n");

    std::string server_addr;
    JString2String(env, addr, server_addr);
    websocket_run(server_addr.c_str(), websocket_callback, env, obj);
    log_info("FINISH!\r\n");
}
