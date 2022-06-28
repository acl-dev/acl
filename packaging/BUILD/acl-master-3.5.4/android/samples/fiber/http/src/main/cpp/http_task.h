//
// Created by shuxin 　　zheng on 2020/10/4.
//

#pragma once
#include <jni.h>

class http_task {
public:
    http_task(JNIEnv* env, const char* host, int port, const char* url);
    ~http_task(void);

    http_task* wait(void);
    void done(void);

public:
    const acl::string& body(void) const {
        return body_;
    }

    acl::string& body(void) {
        return body_;
    }

    JNIEnv* get_env(void) {
        return env_;
    }

    const acl::string& get_server_host(void) const {
        return server_host_;
    }

    int get_server_port(void) const {
        return server_port_;
    }

    const acl::string& get_server_url(void) const {
        return server_url_;
    }

public:
    void debug(void) const;

private:
    JNIEnv* env_;
    acl::string server_host_;
    int server_port_;
    acl::string server_url_;
    acl::string body_;

private:
    acl::fiber_tbox<http_task> box_;
};

