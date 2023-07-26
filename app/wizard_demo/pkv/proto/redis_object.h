//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#pragma once

#include <string>
#include <vector>
#include "redis_type.h"

namespace pkv {

class redis_object;
using shared_redis = std::shared_ptr<redis_object>;

typedef enum {
	REDIS_OBJ_UNKOWN,
	REDIS_OBJ_NIL,
	REDIS_OBJ_ERROR,
	REDIS_OBJ_STATUS,
	REDIS_OBJ_INTEGER,
	REDIS_OBJ_STRING,
	REDIS_OBJ_ARRAY,
} redis_obj_t;

class redis_object {
public:
    explicit redis_object(std::vector<redis_object*>& cache, size_t cache_max);
    ~redis_object();

    redis_object(const redis_object&) = delete;
    void operator=(const redis_object&) = delete;

    void set_parent(redis_object* parent);
    void reset();

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

    [[nodiscard]] redis_obj_t get_type() const {
        return type_;
    }

    [[nodiscard]] const char* get_cmd() const;

    [[nodiscard]] const char* get_str() const;

    [[nodiscard]] const std::vector<redis_object*>& get_objects() const {
        return objs_;
    }

public:
    redis_object& set_status(const std::string& data, bool return_parent = false);
    redis_object& set_error(const std::string& data, bool return_parent = false);
    redis_object& set_number(int n, bool return_parent = false);
    redis_object& set_string(const std::string& data, bool return_parent = false);
    redis_object& create_child();

    bool to_string(acl::string& out) const;

private:
    int status_ = redis_s_begin;
    redis_obj_t type_ = REDIS_OBJ_UNKOWN;

    redis_object* parent_ = nullptr;
    redis_object* obj_ = nullptr;

    std::string buf_;
    int cnt_ = 0;

    size_t cache_max_;
    std::vector<redis_object*>& cache_;
    std::vector<redis_object*> objs_;

private:
    const char* get_line(const char*, size_t&, std::string&, bool&);
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
