#include <string>
#include <unistd.h>
#include "acl_cpp/lib_acl.hpp"
#include "fiber/lib_fiber.hpp"
#include "HttpTest.hpp"

class fiber_http : public acl::fiber
{
public:
    fiber_http(const char* addr, const char* url) : addr_(addr), url_(url) {}
    
    // @override
    void run(void) {
        acl::http_request request(addr_);
        acl::http_header& hdr = request.request_header();
        hdr.set_url(url_).set_host(addr_);
        if (!request.request(NULL, 0)) {
            delete this;
            return;
        }
        acl::string body;
        if (request.get_body(body)) {
            printf("%s\r\n", body.c_str());
        } else {
            printf("get_body error=%s\r\n", acl::last_serror());
        }
        delete this;
    }

private:
    acl::string addr_;
    acl::string url_;
    ~fiber_http(void) {}
};

#include <thread>

static void fiber_thread(acl::string* addr, acl::string* url)
{
    acl::fiber* fb = new fiber_http(addr->c_str(), url->c_str());
    delete addr;
    delete url;
    fb->start(128000);
    acl::fiber::schedule();
}

bool http_test(const char* url)
{
    static bool __inited = false;
    if (!__inited) {
        acl::acl_cpp_init();
        __inited = true;
    }

    acl::string* addr_buf = new acl::string("www.baidu.com:80");
    acl::string* url_buf  = new acl::string("/");
    std::thread thread(fiber_thread, addr_buf, url_buf);
    thread.detach();

	return true;
}
