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

static bool HttpGet(
        const std::string& server_addr,
        const std::string& server_host,
        const std::string& server_url,
        acl::string& out)
{
    //acl::http_request request(server_addr.c_str());
    acl::http_request request("61.135.185.32:80");
    acl::http_header& header = request.request_header();
    header.set_url(server_url.c_str())
        .set_host(server_host.c_str())
        .accept_gzip(true)
        .set_keep_alive(false);

    log_info("HttpGet: addr=%s, host=%s, url=%s", server_addr.c_str(),
            server_host.c_str(), server_url.c_str());

    if (!request.request(NULL, 0)) {
        log_error("send http request error=%s", acl::last_serror());
        return false;
    }

    if (!request.get_body(out)) {
        log_error("get body error: %s", acl::last_serror());
        return false;
    }

    return true;
}

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

    acl::string body;
    if (!HttpGet(server_addr, server_host, server_url, body)) {
        return NULL;
    }

    return String2JString(env, body);
}

class fiber_sleep : public acl::fiber {
public:
    fiber_sleep(void) {}

protected:
    ~fiber_sleep(void) {}

    // @override
    void run(void) {
        for (int i = 0; i < 1; i++) {
            log_info("begin sleep ...");
            sleep(2);
            log_info("sleep wakeup...");
        }
        delete this;
    }
};

extern "C" JNIEXPORT void JNICALL
Java_com_example_http_HttpFiberThread_FiberSchedule(
        JNIEnv* env,
        jobject me)
{
    log_info(">>>>>>fiber schedule now<<<<");
    acl::fiber* fb = new fiber_sleep;
    fb->start();
    acl::fiber::schedule();
    log_info("===============fiber schedule stopped==========");
}

class http_fiber : public acl::fiber {
public:
    http_fiber(const char* addr, const char* host, const char* url)
    : addr_(addr), host_(host), url_(url)
    {}

protected:
    // @override
    void run(void) {
        acl::string body;
        log_info("in fiber HttpGet: addr=%s, host=%s, url=%s",
                addr_.c_str(), host_.c_str(), url_.c_str());

        if (HttpGet(addr_, host_, url_, body)) {
            //log_info("%s", body.c_str());
            log_info("get one, body size=%ld", (long) body.size());
        } else {
            log_error("HttpGet error, addr=%s, host=%s, url=%s",
                    addr_.c_str(), host_.c_str(), url_.c_str());
        }

        delete this;
    }

private:
    std::string addr_;
    std::string host_;
    std::string url_;

    ~http_fiber(void) {}
};

extern "C" JNIEXPORT void JNICALL
Java_com_example_http_HttpFiberThread_HttpFiberGet(
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
    acl::fiber* fb = new http_fiber(server_addr.c_str(),
            server_host.c_str(), server_url.c_str());
    fb->start();
}
