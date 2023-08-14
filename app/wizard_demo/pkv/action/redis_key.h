//
// Created by shuxin ¡¡¡¡zheng on 2023/7/31.
//

#pragma once
#include "redis_command.h"

namespace pkv {

class redis_handler;
class redis_object;
class redis_coder;

class redis_key : public redis_command {
public:
    redis_key(redis_handler& handler, const redis_object& obj);
    ~redis_key() override = default;

    bool del(redis_coder& result);
    bool type(redis_coder& result);
};

} // namespace pkv
