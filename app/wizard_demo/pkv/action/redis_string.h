//
// Created by shuxin ¡¡¡¡zheng on 2023/7/31.
//

#pragma once
#include "dao/db.h"

namespace pkv {

class redis_object;
class redis_coder;

class redis_string {
public:
    redis_string(shared_db& db, const redis_object &obj, redis_coder& base);
    ~redis_string() = default;

    bool set(redis_coder& result);
    bool get(redis_coder& result);

private:
    shared_db& db_;
    const redis_object &obj_;
    redis_coder& base_;
};

} // namespace pkv
