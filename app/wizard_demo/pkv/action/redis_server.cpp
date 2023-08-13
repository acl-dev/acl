//
// Created by shuxin ¡¡¡¡zheng on 2023/8/2.
//

#include "stdafx.h"
#include "coder/redis_coder.h"
#include "coder/redis_object.h"

#include "redis_handler.h"
#include "redis_server.h"

namespace pkv {

#define EQ  !strcasecmp

redis_server::redis_server(redis_handler &handler, const redis_object &obj)
: redis_command(handler, obj)
{}

bool redis_server::config(redis_coder &result) {
    if (obj_.size() < 3) {
        logger_error("invalid CONFIG command's size=%zd < 3", obj_.size());
        return false;
    }

    auto oper = obj_[1];
    if (oper == nullptr || *oper == 0) {
        logger_error("key null");
        return false;
    }

    auto name = obj_[2];
    if (name == nullptr || *name == 0) {
        logger_error("name null");
        return false;
    }

    if (EQ(oper, "GET")) {
        return config_get(name, result);
    }

    return false;
}

bool redis_server::config_get(const char *name, redis_coder &result) {
    if (EQ(name, "save")) {
        return config_get_save(result);
    } else if (EQ(name, "appendonly")) {
        return config_get_appendonly(result);
    }

    result.create_object().set_error("unkown config entry");
    return false;
}

bool redis_server::config_get_save(redis_coder &result) {
    result.create_object().create_child().set_string("save", true)
        .create_child().set_string("3600 1 300 100 60 10000");
    return true;
}

bool redis_server::config_get_appendonly(redis_coder &result) {
    result.create_object().create_child().set_string("appendonly", true)
        .create_child().set_string("no");
    return true;
}

} // namespace pkv
