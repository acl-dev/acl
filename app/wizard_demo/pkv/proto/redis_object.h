//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#pragma once

#include "redis_type.h"

namespace pkv {

class redis_object;
using shared_redis = std::shared_ptr<redis_object>;

class redis_object {
public:
    redis_object(acl::dbuf_pool* dbuf, redis_object* parent);
    ~redis_object() = default;

    const char* update(const char* data, size_t& len);

    bool finish() const {
        return status_ == redis_s_finish;
    }

    bool failed() const {
        return status_ == redis_s_null;
    }

    int get_status() const {
        return status_;
    }

    acl::redis_result_t get_type() const {
        return rr_ ? rr_->get_type() : acl::REDIS_RESULT_UNKOWN;
    }

public:
    bool to_string(acl::string& out) const;

private:
    acl::dbuf_pool* dbuf_;
    redis_object* parent_;
    int status_;

    int cnt_;
    std::string buf_;
    acl::redis_result* rr_;
    std::vector<shared_redis> objs_;
    shared_redis obj_;

private:
    static void put_data(acl::dbuf_pool*, acl::redis_result*, const char*, size_t);
    const char* get_line(const char*, size_t&, bool&);
    const char* get_length(const char*, size_t&, int&);
    const char* get_data(const char*, size_t&, size_t);

    const char* parse_object(const char*, size_t&);

public:
    const char* parse_null(const char* data, size_t& len);
    const char* parse_begin(const char* data, size_t& len);
    const char* parse_status(const char* data, size_t& len);
    const char* parse_error(const char* data, size_t& len);
    const char* parse_number(const char* data, size_t& len);
    const char* parse_strlen(const char* data, size_t& len);
    const char* parse_string(const char* data, size_t& len);
    const char* parse_strend(const char* data, size_t& len);
    const char* parse_arlen(const char* data, size_t& len);
    const char* parse_array(const char* data, size_t& len);
};

} // namespace pkv
