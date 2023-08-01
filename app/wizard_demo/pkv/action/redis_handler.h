//
// Created by zsx on 2023/7/23.
//

#pragma once

#include <vector>
#include "proto/redis_coder.h"
#include "dao/db.h"

namespace pkv {

class redis_object;

class redis_handler {
public:
    explicit redis_handler(shared_db& db, redis_coder& parser,
        acl::socket_stream& conn);
    ~redis_handler() = default;

    bool handle();

    std::vector<redis_object*>& get_cache() const {
        return parser_.get_cache();
    }

    redis_coder& get_coder() {
        return coder_;
    }

    shared_db& get_db() {
        return db_;
    }

private:
    shared_db& db_;
    redis_coder& parser_;
    acl::socket_stream& conn_;
    redis_coder builder_;
    redis_coder coder_;

    bool handle_one(const redis_object& obj);

    bool hset(const redis_object& obj);
    bool hget(const redis_object& obj);
    bool hdel(const redis_object& obj);
    bool hmset(const redis_object& obj);
    bool hmget(const redis_object& obj);
    bool hgetall(const redis_object& obj);
};

}
