//
// Created by shuxin 　　zheng on 2020/10/4.
//

#pragma once

class http_task;

class http_get {
public:
    http_get(http_task* t);
    ~http_get(void) {}

    void run(void);

private:
    http_task* task_;

    bool get(const char* addr, const char* host,
            const char* url, acl::string& out);
};

