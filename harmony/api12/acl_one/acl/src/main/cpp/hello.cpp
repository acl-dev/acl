#include <napi/native_api.h>
#include <hilog/log.h>
#include "mystdafx.h"
#include "logger.h"

class http_thread : public acl::thread {
public:
    http_thread(const char* url) : url_(url) {}
    ~http_thread() = default;
    
    const acl::string& get_body() const {
        return body_;
    }
    
    const acl::string& get_head() const {
        return head_;;
    }

 protected:
    std::string url_;
    acl::string head_;
    acl::string body_;

    // @override
    void *run() override {
        url_get();
        return nullptr;
    }
    
    bool url_get() {
        const char* addr = "www.baidu.com:80";
        acl::http_request req(addr, 5);
        req.request_header().set_url("/")
            .set_host("www.baidu.com");

        if (!req.request(nullptr, 0)) {
            log_info("%s: http request error, addr=%s", __func__, addr);
            return false;
        }
        
        int http_status = req.http_status();
        acl::string buf;
        buf.format("http status=%d", http_status);
        log_info( "%s: url_get", __func__, buf.c_str());

        if (!req.get_body(body_)) {
            log_error("%s: get http body error!", __func__);
            return false;
        }

        req.get_client()->sprint_header(head_);
        log_info("%s: http response body: %s", __func__, body_.c_str());
        return true;
    }
};

static napi_value HttpGet(napi_env env, napi_callback_info info)
{
    log_info("%s: Started!", __func__);

    size_t argc = 1;
    napi_value args[1] = { nullptr };
    
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    char url[256];
    size_t len;
    napi_status res = napi_get_value_string_utf8(env, args[0], url, sizeof(url), &len);    
    if (res != napi_ok) {
        log_info("%s: napi_get_value_string_utf8 error=%d", __func__, res);
        return nullptr;
    }

    log_info( "%s: url=%s", __func__, url);

    http_thread http(url);
    http.set_detachable(false);
    http.start();
    http.wait();
    
    const acl::string& body = http.get_body();
    acl::string head = http.get_head();

    if (body.empty() || head.empty()) {
        log_info("%s: http reply body or head empty!", __func__);
        return nullptr;
    }
    
    head.format_append("\r\nBody length: %zd\r\n", body.length());

    napi_value result;
    res = napi_create_string_utf8(env, head.c_str(), head.size(), &result);
    if (res != napi_ok) {
        log_info("%s: napi_create_string_utf8 error=%d", __func__, res);
        return nullptr;
    }
    
    log_info("%s: At last, http body length=%zd", __func__, body.size());
    return result;
}

static napi_value Add(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args , nullptr, nullptr);

    napi_valuetype valuetype0;
    napi_typeof(env, args[0], &valuetype0);

    napi_valuetype valuetype1;
    napi_typeof(env, args[1], &valuetype1);

    double value0;
    napi_get_value_double(env, args[0], &value0);

    double value1;
    napi_get_value_double(env, args[1], &value1);

    napi_value sum;
    napi_create_double(env, value0 + value1, &sum);

    return sum;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    acl::acl_cpp_init();
    log_open();

    const char* version = acl_version();
    log_info("%s: Acl version=%s", __func__, version);

    napi_property_descriptor desc[] = {
        { "HttpGet", nullptr, HttpGet, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "Add",     nullptr, Add,     nullptr, nullptr, nullptr, napi_default, nullptr },
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version =1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&demoModule);
}
