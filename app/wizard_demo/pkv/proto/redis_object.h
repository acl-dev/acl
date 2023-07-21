//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#pragma once

#include "redis_type.h"

namespace pkv {

//class redis_object;
//using shared_redis = std::shared_ptr<redis_object>;

class redis_object {
public:
    explicit redis_object(acl::dbuf_pool* dbuf, redis_object* parent);

    redis_object(const redis_object&) = delete;
    void operator=(const redis_object&) = delete;

    void *operator new(size_t size, acl::dbuf_pool* pool);
    void operator delete(void* ptr, acl::dbuf_pool* pool);

private:
    ~redis_object() = default;

public:
    const char* update(const char* data, size_t& len);

    [[nodiscard]] bool finish() const {
        return status_ == redis_s_finish;
    }

    [[nodiscard]] bool failed() const {
        return status_ == redis_s_null;
    }

    [[nodiscard]] int get_status() const {
        return status_;
    }

    [[nodiscard]] acl::redis_result_t get_type() const {
        return me_ ? me_->get_type() : acl::REDIS_RESULT_UNKOWN;
    }

public:
    redis_object& set_status(const std::string& data, bool return_parent = false);
    redis_object& set_error(const std::string& data, bool return_parent = false);
    redis_object& set_number(int n, bool return_parent = false);
    redis_object& set_string(const std::string& data, bool return_parent = false);
    redis_object& create_child();

    bool to_string(acl::string& out) const;

private:
    acl::dbuf_pool* dbuf_;
    redis_object* parent_;
    int status_ = redis_s_begin;
    acl::redis_result* me_ = nullptr;
    std::vector<redis_object*> objs_;

    std::string buf_;
    int cnt_ = 0;
    redis_object* obj_ = nullptr;

private:
    static void put_data(acl::dbuf_pool*, acl::redis_result*, const char*, size_t);
    const char* get_line(const char*, size_t&, bool&);
    const char* get_length(const char*, size_t&, int&, bool&);
    const char* get_data(const char*, size_t&, size_t);

    const char* parse_object(const char*, size_t&);

public:
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
