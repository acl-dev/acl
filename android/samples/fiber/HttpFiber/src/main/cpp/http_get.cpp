//
// Created by shuxin 　　zheng on 2020/10/4.
//

#include "stdafx.h"
#include "log.h"
#include "http_get.h"

http_get::http_get(const char* host, int port, const char* url)
: host_(host)
, port_(port)
, url_(url)
{}

bool http_get::get(const char* addr, const char* host,
        const char *url, acl::string &out) {
    acl::http_request request(addr);
    acl::http_header& header = request.request_header();

    header.set_url(url)
            .set_host(host)
            .accept_gzip(true)
            .set_keep_alive(false);

    log_info("http_get(%d): addr=%s, host=%s, url=%s", __LINE__, addr, host, url);

    if (!request.request(NULL, 0)) {
        log_error("send http request error=%s", acl::last_serror());
        return false;
    }

    if (!request.get_body(out)) {
        log_error("get body error: %s", acl::last_serror());
        return false;
    }

    acl::http_client* conn = request.get_client();
    acl::string hdr_res;
    conn->sprint_header(hdr_res, "response header");
    log_info("%s", hdr_res.c_str());
    return true;
}

void http_get::run(void) {
    std::vector<acl::string> ips;

    const char* host = host_.c_str();
    int err = 0;
    go_wait_thread[&] {
        log_info("begin resolve host=%s", host);
        // resolve domain in a thread to avoid blocking the fiber thread scheule.
#if 1
        struct hostent* ent = gethostbyname(host);
        if (ent == NULL) {
            err = h_errno;
            log_error("can't get addr for host=%s, %s", host, hstrerror(h_errno));
            return;
        }
        char ip[64];
        for (char** pptr = ent->h_addr_list; *pptr != NULL; pptr++) {
            if (inet_ntop(ent->h_addrtype, *pptr, ip, sizeof(ip))) {
                ips.push_back(ip);
                log_info("http_get::run: one ip: %s", ip);
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
        acl::string buf;
        buf.format("can't get addr=%s, error=%s", host, hstrerror(err));
        return;
    }

    for (std::vector<acl::string>::const_iterator cit = ips.begin();
         cit != ips.end(); ++cit) {
        acl::string addr;
        addr.format("%s:%d", (*cit).c_str(), port_);
        acl::string buf;
        if (get(addr, host, url_.c_str(), buf)) {
            break;
        } else {
            buf.clear();
        }
    }
}