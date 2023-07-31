//
// Created by zsx on 2023/7/23.
//

#pragma once

#include "proto/redis_coder.h"
#include "dao/db.h"

namespace pkv {

class redis_object;

class redis_handler {
public:
    explicit redis_handler(shared_db& db, const redis_coder& parser,
        acl::socket_stream& conn);
    ~redis_handler() = default;

    bool handle();

private:
    shared_db& db_;
    const redis_coder& parser_;
    redis_coder builder_;
    redis_coder coder_;
    acl::socket_stream& conn_;

    bool handle_one(const redis_object& obj);

    bool set(const redis_object& obj);
    bool get(const redis_object& obj);
    bool del(const redis_object& obj);
    bool type(const redis_object& obj);
    bool hset(const redis_object& obj);
    bool hget(const redis_object& obj);
    bool hdel(const redis_object& obj);
    bool hmset(const redis_object& obj);
    bool hmget(const redis_object& obj);
    bool hgetall(const redis_object& obj);
};

}
