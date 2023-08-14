//
// Created by shuxin ¡¡¡¡zheng on 2023/7/31.
//

#pragma once
#include "redis_command.h"

namespace pkv {

class redis_coder;

class redis_string : public redis_command {
public:
    redis_string(redis_handler& handler, const redis_object& obj);
    ~redis_string() override = default;

    bool set(redis_coder& result);
    bool get(redis_coder& result);

};

} // namespace pkv