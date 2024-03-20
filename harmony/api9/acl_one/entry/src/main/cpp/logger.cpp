//
// Created by zhengshuxin on 2019/8/22.
//

#include "mystdafx.h"
#include <napi/native_api.h>
#include <hilog/log.h>
#include "logger.h"

#define LOG_ON

void log_info(const char* fmt, ...)
{
    const char* ver = acl_version();
    char tag[256];
    snprintf(tag, sizeof(tag), "acl-%s", ver);
    acl::string buf;

    va_list ap;
    va_start(ap, fmt);
    buf.vformat(fmt, ap);
    va_end(ap);

    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "tag", "%{public}s", buf.c_str());
}

void log_error(const char* fmt, ...)
{
    const char* ver = acl_version();
    char tag[256];
    snprintf(tag, sizeof(tag), "acl-%s", ver);
    acl::string buf;

    va_list ap;
    va_start(ap, fmt);
    buf.vformat(fmt, ap);
    va_end(ap);

    OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_DOMAIN, tag, "%{public}s", buf.c_str());
}

void log_warn(const char* fmt, ...)
{
    const char* ver = acl_version();
    char tag[256];
    snprintf(tag, sizeof(tag), "acl-%s", ver);
    acl::string buf;

    va_list ap;
    va_start(ap, fmt);
    buf.vformat(fmt, ap);
    va_end(ap);

    OH_LOG_Print(LOG_APP, LOG_WARN, LOG_DOMAIN, tag, "%{public}s", buf.c_str());
}

void log_fatal(const char* fmt, ...)
{
    const char* ver = acl_version();
    char tag[256];
    snprintf(tag, sizeof(tag), "acl-%s", ver);
    acl::string buf;

    va_list ap;
    va_start(ap, fmt);
    buf.vformat(fmt, ap);
    va_end(ap);

    OH_LOG_Print(LOG_APP, LOG_FATAL, LOG_DOMAIN, tag, "%{public}s", buf.c_str());
}

void log_debug(const char* fmt, ...)
{
    const char* ver = acl_version();
    char tag[256];
    snprintf(tag, sizeof(tag), "acl-%s", ver);
    acl::string buf;

    va_list ap;
    va_start(ap, fmt);
    buf.vformat(fmt, ap);
    va_end(ap);

    OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_DOMAIN, tag, "%{public}s", buf.c_str());
}

static int open_callback(const char*, void*) {
    return 0;
}

static write_fn s_write_callback;

static void write_callback(void*, const char* str)
{
    log_info("%s", str);
}

static void log_callback(void* ctx, const char* fmt, va_list ap) {
    static acl::string* buf = nullptr;
    if (buf == nullptr) {
        buf = new acl::string(512);
    }
    buf->vformat(fmt, ap);
    s_write_callback(ctx, buf->c_str());
}

static void init_once() {
    s_write_callback = write_callback;
    acl_msg_register(open_callback, nullptr, log_callback, nullptr);
    acl_msg_open("dummy.txt", "dummy");
}

static acl_pthread_once_t s_init_once = ACL_PTHREAD_ONCE_INIT;

void log_open()
{
    if (acl_pthread_once(&s_init_once, init_once) != 0) {
        return;
    }

    //log_info("call logger_hook ok");
}

void log_close()
{
}