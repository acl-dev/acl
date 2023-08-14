//
// Created by shuxin ¡¡¡¡zheng on 2023/8/2.
//

#pragma once
#include "redis_command.h"

namespace pkv {

class redis_coder;

class redis_server : public redis_command {
public:
    redis_server(redis_handler &handler, const redis_object &obj);
    ~redis_server() override = default;

    bool config(redis_coder &result);

private:
    static bool config_get(const char *name, redis_coder &result);
    static bool config_get_save(redis_coder &result);
    static bool config_get_appendonly(redis_coder &result);
};

} // namespace pkv
