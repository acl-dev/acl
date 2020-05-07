//
// Created by shuxin 　　zheng on 2020-03-29.
//

#include "stdafx.h"
#include "util.h"
#include "websocket.h"

extern "C" JNIEXPORT void JNICALL
Java_com_example_http_HttpClient_WebsocketStart(
        JNIEnv*,
        jobject)
{
    log_info("START!\r\n");
    websocket_run();
    log_info("FINISH!\r\n");
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
