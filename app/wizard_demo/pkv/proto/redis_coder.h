//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#pragma once

#include "redis_object.h"

namespace pkv {

class redis_coder {
public:
    redis_coder();
    ~redis_coder();

    const char* update(const char* data, size_t& len);

    [[nodiscard]] const std::vector<redis_object*>& get_objects() const {
        return objs_;
    }

    [[nodiscard]] redis_object* get_curr() const {
        return curr_;
    }

    void clear();

public:
    [[nodiscard]] redis_object& create_object();

    bool to_string(acl::string& out) const;

private:
    acl::dbuf_pool* dbuf_;
    std::vector<redis_object*> objs_;
    redis_object* curr_;
};

bool test_redis_parse(const char* filepath);
bool test_redis_parse_once(const char* filepath);
bool test_redis_parse_stream(const char* filepath);

bool test_redis_build();

} // namespace pkv
