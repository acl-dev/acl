//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#pragma once

#include "redis_object.h"

namespace pkv {

class redis_parser {
public:
    redis_parser();
    ~redis_parser() = default;

    size_t update(const char* data, size_t len);

private:
    int status_;
    std::vector<shared_redis> objs_;
    shared_redis curr_;
};

} // namespace pkv
