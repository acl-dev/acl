//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#pragma once

#include "redis_object.h"

namespace pkv {

class redis_parser {
public:
    redis_parser();
    ~redis_parser();

    const char* update(const char* data, size_t& len);

    bool to_string(acl::string& out) const;

private:
    acl::dbuf_pool* dbuf_;
    std::vector<shared_redis> objs_;
    shared_redis curr_;
};

bool test_redis_parse(const char* filepath);

} // namespace pkv
