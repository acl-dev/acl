//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#pragma once

namespace pkv {

class redis_object;
using shared_redis = std::shared_ptr<redis_object>;

class redis_object {
public:
    redis_object();
    ~redis_object();

    size_t update(const char* data, size_t len);

    bool finish() const {
        return finish_;
    }

    int get_status(void) const {
        return status_;
    }

private:
    bool finish_;
    int status_;
    acl::dbuf_pool* dbuf_;
    std::string cmd_;
    std::vector<std::string> items_;

    int len_;
    std::string buf_;
    acl::redis_result* rr_;
    std::vector<shared_redis> objs_;
    shared_redis obj_;

private:
    void put_data(acl::dbuf_pool*, acl::redis_result*, const char*, size_t);
    size_t get_line(const char*, size_t, bool&);
    size_t get_length(const char*, size_t, int&);
    size_t get_data(const char*, size_t, size_t);
    size_t get_object(const char*, size_t, shared_redis);

public:
    size_t parse_null(const char* data, size_t len);
    size_t parse_obj(const char* data, size_t len);
    size_t parse_arlen(const char* data, size_t len);
    size_t parse_array(const char* data, size_t len);
    size_t parse_number(const char* data, size_t len);
    size_t parse_strlen(const char* data, size_t len);
    size_t parse_string(const char* data, size_t len);
    size_t parse_status(const char* data, size_t len);
    size_t parse_error(const char* data, size_t len);
};

} // namespace pkv
