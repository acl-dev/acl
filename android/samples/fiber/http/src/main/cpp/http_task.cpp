//
// Created by shuxin 　　zheng on 2020/10/4.
//

#include "stdafx.h"
#include "log.h"
#include "http_task.h"

http_task::http_task(JNIEnv *env, const char *host, int port,
    const char *url)
: env_(env)
, server_host_(host)
, server_port_(port)
, server_url_(url)
{}

http_task::~http_task(void) {}

void http_task::debug(void) const {
    log_info(">>task=%p: host=%s, port=%d, url=%s<<",
            this, server_host_.c_str(), server_port_, server_url_.c_str());
}

http_task* http_task::wait(void) {
    return box_.pop();
}

void http_task::done(void) {
    box_.push(this);
}

