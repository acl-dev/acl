//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#pragma once

#include <string>
#include <vector>
#include "c++_patch.h"
#include "redis_type.h"

namespace pkv {

typedef enum {
	REDIS_OBJ_UNKOWN,
	REDIS_OBJ_ERROR,
	REDIS_OBJ_STATUS,
	REDIS_OBJ_INTEGER,
	REDIS_OBJ_STRING,
	REDIS_OBJ_ARRAY,
} redis_obj_t;

class redis_ocache;

class redis_object {
public:
    explicit redis_object(redis_ocache& cache);

    void destroy();

    redis_object(const redis_object&) = delete;
    void operator=(const redis_object&) = delete;

    void set_parent(redis_object* parent);
    void reset();

//private:
    ~redis_object();

public:
    const char* update(const char* data, size_t& len);

    NODISCARD bool finish() const {
        return status_ == redis_s_finish;
    }

    NODISCARD bool failed() const {
        return status_ == redis_s_null;
    }

    NODISCARD int get_status() const {
        return status_;
    }

    NODISCARD redis_obj_t get_type() const {
        return type_;
    }

    NODISCARD const char* get_cmd() const;

    NODISCARD const char* get_str() const;

    NODISCARD size_t size() const {
        return objs_ ? objs_->size() : 0;
    }

    NODISCARD const char* operator[](size_t i) const;

public:
    redis_object& set_status(const std::string& data, bool return_parent = false);
    redis_object& set_error(const std::string& data, bool return_parent = false);
    redis_object& set_number(long long n, bool return_parent = false);
    redis_object& set_string(const std::string& data, bool return_parent = false);
    redis_object& create_child();

    bool to_string(std::string& out) const;

private:
    int status_ = redis_s_begin;
    redis_obj_t type_ = REDIS_OBJ_UNKOWN;

    redis_object* parent_ = nullptr;
    redis_object* obj_ = nullptr;

    std::string buf_;
    int cnt_ = 0;

    redis_ocache& cache_;
    std::vector<redis_object*>* objs_;

private:
    static const char* get_line(const char*, size_t&, std::string&, bool&);
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
