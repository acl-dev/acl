//
// Created by shuxin 　　zheng on 2020/10/4.
//

#pragma once

class http_get {
public:
    http_get( const char* host, int port, const char* url);
    ~http_get(void) {}

    void run(void);

private:
    acl::string host_;
    int port_;
    acl::string url_;

    bool get(const char* addr, const char* host,
            const char* url, acl::string& out);
};

