//
// Created by shuxin ¡¡¡¡zheng on 2023/7/31.
//

#pragma once
#include "dao/db.h"
#include "proto/redis_coder.h"

namespace pkv {

class redis_object;
class redis_coder;

class redis_key {
public:
    redis_key(shared_db& db, const redis_object& obj, redis_coder& base);
    ~redis_key() = default;

    bool del(redis_coder& result);
    bool type(redis_coder& result);

private:
    shared_db& db_;
    const redis_object& obj_;
    redis_coder& base_;
};

} // namespace pkv
