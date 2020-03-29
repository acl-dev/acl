//
// Created by shuxin 　　zheng on 2020-03-29.
//

#include "stdafx.h"

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

static void log_info(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, "dns-info", fmt, ap);
    va_end(ap);
}

static void log_error(const char* fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_ERROR, "dns-error", fmt, ap);
    va_end(ap);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_http_HttpClient_HttpGet(
        JNIEnv* env,
        jobject me,
        jstring addr,
        jstring host,
        jstring url)
{
    std::string server_addr, server_host, server_url;
    JString2String(env, addr, server_addr);
    JString2String(env, host, server_host);
    JString2String(env, url, server_url);

    acl::http_request request(server_addr.c_str());
    acl::http_header& header = request.request_header();
    header.set_url(server_url.c_str())
        .set_host(server_host.c_str())
        .accept_gzip(true)
        .set_keep_alive(false);

    log_info("addr=%s, host=%s, url=%s", server_addr.c_str(),
            server_host.c_str(), server_url.c_str());

    if (!request.request(NULL, 0)) {
        log_error("send http request error=%s", acl::last_serror());
        return NULL;
    }

    acl::string body;
    if (!request.get_body(body)) {
        log_error("get body error: %s", acl::last_serror());
        return NULL;
    }

    return String2JString(env, body);
}
