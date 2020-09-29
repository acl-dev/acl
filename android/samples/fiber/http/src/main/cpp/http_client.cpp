//
// Created by shuxin 　　zheng on 2020-03-29.
//

#include "stdafx.h"
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

static bool http_get(
        const std::string& server_addr,
        const std::string& server_host,
        const std::string& server_url,
        acl::string& out)
{
#if 0
    acl::http_request request(server_addr.c_str());
#else
    acl::http_request request("61.135.185.32:80");
#endif
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

////////////////////////////////////////////////////////////////////////////////

class task {
public:
    task(JNIEnv* env, const char* addr, const char* host, const char* url)
    : env_(env)
    , server_addr_(addr)
    , server_host_(host)
    , server_url_(url)
    , body_(NULL) {}
    ~task(void) {
        delete body_;
    }

    void push_result() {
        box_.push(this);
    }

    task* wait_result() {
        return box_.pop();
    }

    void set_body(acl::string* body) {
        body_ = body;
    }

    acl::string* get_body(void) {
        return body_;
    }

    JNIEnv* get_env(void) {
        return env_;
    }

    const std::string& get_server_addr(void) const {
        return server_addr_;
    }

    const std::string& get_server_host(void) const {
        return server_host_;
    }

    const std::string& get_server_url(void) const {
        return server_url_;
    }

public:
    void debug(void) const {
        log_info(">>task=%p: addr=%s, host=%s, url=%s<<", this,
                server_addr_.c_str(), server_host_.c_str(), server_url_.c_str());
        const char* name = "com/example/http/HttpFiberThread";
        jclass clazz = env_->FindClass(name);
        log_info(">>task=%p, env=%p, clazz=%p<<", this, env_, clazz);
    }

private:
    JNIEnv* env_;
    std::string server_addr_;
    std::string server_host_;
    std::string server_url_;

private:
    acl::tbox<task> box_;
    acl::string* body_;
};

static void http_get(task* t) {
    acl::string* body = new acl::string;

    //t->debug();

    log_info("http_get(%d): addr=%s, host=%s, url=%s", __LINE__,
            t->get_server_addr().c_str(),
            t->get_server_host().c_str(), t->get_server_url().c_str());

    if (http_get(t->get_server_addr(), t->get_server_host(),
            t->get_server_url(), *body)) {
        t->set_body(body);
    } else {
        delete body;
    }

    t->push_result();
}

class fiber_waiter : public acl::fiber {
public:
    fiber_waiter(void) {
        box_ = new acl::fiber_tbox<task>;
    }
    ~fiber_waiter(void) {}

    void push(task* t) {
        box_->push(t);
    }

protected:
    // @override
    void run(void) {
        while (true) {
            log_info(">>>waiter fiber started!");
            task* t = box_->pop();
            go[=] {
                http_get(t);
            };
        }
    }

private:
    acl::fiber_tbox<task>* box_;
};

class fiber_sleep : public acl::fiber {
public:
    fiber_sleep(void) {}
    ~fiber_sleep(void) {}

protected:
    void run(void) {
        while (true) {
            sleep(2);
            //log_info(">>sleep wakeup<<<");
        }
    }
};

static void fiber_thread_run(acl::fiber* waiter, acl::fiber* sleeper) {
    waiter->start(256000);
    sleeper->start(128000);

    log_info(">>>>>>fiber schedule now<<<<");
    acl::fiber::schedule();
    log_info("===============fiber schedule stopped==========");
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_http_HttpFiberThread_FiberSchedule(
        JNIEnv* env,
        jobject me)
{
    log_open();

    acl::fiber* waiter = new fiber_waiter;
    acl::fiber* sleeper = new fiber_sleep;
    std::thread* thread = new std::thread(fiber_thread_run, waiter, sleeper);
    thread->detach();
    return (jlong) waiter;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_http_HttpFiberThread_HttpGet(
        JNIEnv* env,
        jobject me,
        jlong o,
        jstring addr,
        jstring host,
        jstring url)
{
    fiber_waiter* waiter = (fiber_waiter*) o;

    std::string server_addr, server_host, server_url;
    JString2String(env, addr, server_addr);
    JString2String(env, host, server_host);
    JString2String(env, url, server_url);

    log_info(">>>HttpGet: addr=%s, host=%s, url=%s",
            server_addr.c_str(), server_host.c_str(), server_url.c_str());

    task t(env, server_addr.c_str(), server_host.c_str(), server_url.c_str());
    t.debug();

    waiter->push(&t);
    t.wait_result();
    acl::string* s = t.get_body();
    if (s == NULL) {
        return NULL;
    }

    return String2JString(env, s->c_str());
}
