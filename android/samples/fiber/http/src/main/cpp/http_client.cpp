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

static bool http_get(const char* server_addr, const char* server_host,
        const char* server_url, acl::string& out)
{
    acl::http_request request(server_addr);
    acl::http_header& header = request.request_header();

    header.set_url(server_url)
        .set_host(server_host)
        .accept_gzip(true)
        .set_keep_alive(false);

    log_info("HttpGet: addr=%s, host=%s, url=%s",
            server_addr, server_host, server_url);

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
    task(JNIEnv* env, const char* host, int port, const char* url)
    : env_(env)
    , server_host_(host)
    , server_port_(port)
    , server_url_(url)
    , body_(NULL)
    {}

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

    const std::string& get_server_host(void) const {
        return server_host_;
    }

    int get_server_port(void) const {
        return server_port_;
    }

    const std::string& get_server_url(void) const {
        return server_url_;
    }

public:
    void debug(void) const {
        log_info(">>task=%p: host=%s, port=%d, url=%s<<", this,
                server_host_.c_str(), server_port_, server_url_.c_str());
        const char* name = "com/example/http/HttpFiberThread";
        jclass clazz = env_->FindClass(name);
        log_info(">>task=%p, env=%p, clazz=%p<<", this, env_, clazz);
    }

private:
    JNIEnv* env_;
    std::string server_host_;
    int server_port_;
    std::string server_url_;

private:
    acl::fiber_tbox<task> box_;
    acl::string* body_;
};

static void http_get(task* t) {
    //t->debug();

    log_info("thread-%ld: http_get(%d): host=%s, port=%d, url=%s",
            acl::thread::self(), __LINE__,
            t->get_server_host().c_str(), t->get_server_port(),
            t->get_server_url().c_str());

    const char* host = t->get_server_host().c_str();
    std::vector<acl::string> ips;

    int errnum = 0;
    go_wait_thread[&] {
        log_info("thread-%ld: begin resolve host=%s", acl::thread::self(), host);
        // resolve domain in a thread to avoid blocking the fiber thread scheule.
#if 0
        struct hostent* ent = gethostbyname(host);
        if (ent == NULL) {
            errnum = h_errno;
            log_error("thread-%ld: can't get addr for host=%s, %s",
                    acl::thread::self(), host, hstrerror(h_errno));
            return;
        }
        char ip[64];
        for (char** pptr = ent->h_addr_list; *pptr != NULL; pptr++) {
            if (inet_ntop(ent->h_addrtype, *pptr, ip, sizeof(ip))) {
                ips.push_back(ip);
                log_info(">>>one ip: %s", ip);
            }
        }
#else
        ACL_DNS_DB* db = acl_gethostbyname(host, &errnum);
        if (db == NULL) {
            logger_error("gethostbyname error=%s, domain=%s, herr=%d",
                         acl_netdb_strerror(errnum), host, errnum);
            return;
        }
        ACL_ITER iter;
        acl_foreach(iter, db) {
            const ACL_HOSTNAME* hn = (const ACL_HOSTNAME*) iter.data;
            if (hn->saddr.sa.sa_family == AF_INET) {
                ips.push_back(hn->ip);
            } else if (hn->saddr.sa.sa_family == AF_INET6) {
                ips.push_back(hn->ip);
            } else {
                log_info("domain=%s, ipv=%s, type=%d", host, hn->ip,
                   hn->saddr.sa.sa_family);
            }
        }
        acl_netdb_free(db);
#endif
    };

    if (ips.empty()) {
        log_error("resolve domain(%s) failed", host);
        acl::string* buf = new acl::string;
        buf->format("can't get addr=%s, error=%s", host, hstrerror(errnum));
        t->set_body(buf);
        t->push_result();
        return;
    }

    bool success = false;
    acl::string* body = new acl::string;
    for (std::vector<acl::string>::const_iterator cit = ips.begin();
        cit != ips.end(); ++cit) {
        acl::string addr;
        addr.format("%s:%d", (*cit).c_str(), t->get_server_port());
        if (http_get(addr, host, t->get_server_url().c_str(), *body)) {
            success = true;
            break;
        } else {
            body->clear();
        }
    }
    if (success) {
        t->set_body(body);
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
        jstring host,
        jint port,
        jstring url)
{
    fiber_waiter* waiter = (fiber_waiter*) o;

    std::string server_host, server_url;
    JString2String(env, host, server_host);
    JString2String(env, url, server_url);

    log_info(">>>HttpGet: host=%s, port=%d, url=%s",
            server_host.c_str(), port, server_url.c_str());

    task t(env, server_host.c_str(), port, server_url.c_str());
    t.debug();

    waiter->push(&t);
    t.wait_result();
    acl::string* s = t.get_body();
    if (s == NULL) {
        return NULL;
    }

    return String2JString(env, s->c_str());
}
