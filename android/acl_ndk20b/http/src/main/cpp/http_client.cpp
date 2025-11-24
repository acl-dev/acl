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

static bool HttpGet(
        const std::string& server_addr,
        const std::string& server_host,
        const std::string& server_url,
        acl::string& out)
{
    acl::http_request request(server_addr.c_str());
    //acl::http_request request("61.135.185.32:80");
    acl::http_header& header = request.request_header();
    header.set_url(server_url.c_str())
        .set_host(server_host.c_str())
        .accept_gzip(true)
        .set_keep_alive(true);

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

    if (!out.empty()) {
        log_info("response body length: %zd", out.length());
        return true;
    }

    acl::http_client* client = request.get_client();
    if (client != NULL) {
        client->sprint_header(out);
        log_info("http response header: %s", out.c_str());
        return true;
    }

    log_info("get_client NULL");
    int http_status = request.http_status();
    out.format_append("http_status: %d\r\n", http_status);
    const char* ptr = request.header_value("Location");
    if (ptr) {
        out.format_append("Location: %s\r\n", ptr);
    }

    ptr = request.header_value("Content-Length");
    if (ptr) {
        out.format_append("Content-Length: %s\r\n", ptr);
    }

    ptr = request.header_value("Connection");
    if (ptr) {
        out.format_append("Connection: %s\r\n", ptr);
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
