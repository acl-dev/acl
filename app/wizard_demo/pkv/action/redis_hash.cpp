//
// Created by shuxin ¡¡¡¡zheng on 2023/8/1.
//

#include "stdafx.h"
#include "proto/redis_coder.h"
#include "proto/redis_object.h"
#include "dao/hash.h"

#include "redis_handler.h"
#include "redis_hash.h"

namespace pkv {

redis_hash::redis_hash(redis_handler &handler, const redis_object &obj)
: redis_command(handler, obj)
{}

bool redis_hash::hset(redis_coder &result) {
    auto& objs = obj_.get_objects();
    if (objs.size() < 4) {
        logger_error("invalid HSET command's size=%zd < 4", objs.size());
        return false;
    }

    auto key = objs[1]->get_str();
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    auto name = objs[2]->get_str();
    if (name == nullptr || *name == 0) {
        logger_error("name null");
        return false;
    }

    auto value = objs[3]->get_str();
    if (value == nullptr || *value == 0) {
        logger_error("value null");
        return false;
    }

    dao::hash dao;
    if (!dao.hset(handler_.get_db(), key, name, value)) {
        return false;
    }

    result.create_object().set_number(1);
    return true;
}

bool redis_hash::hget(redis_coder &result) {
    auto& objs = obj_.get_objects();
    if (objs.size() < 3) {
        logger_error("invalid HGET command's size=%zd < 3", objs.size());
        return false;
    }

    auto key = objs[1]->get_str();
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    auto name = objs[2]->get_str();
    if (name == nullptr || *name == 0) {
        logger_error("name null");
        return false;
    }

    std::string buff;
    dao::hash dao;
    if (!dao.hget(handler_.get_db(), key, name, buff)) {
        return false;
    }

    //printf(">>hget: [%s]\r\n", buff.c_str());
    result.create_object().set_string(buff);
    return true;
}

bool redis_hash::hdel(redis_coder &result) {
    return false;
}

bool redis_hash::hmset(redis_coder &result) {
    return false;
}

bool redis_hash::hmget(redis_coder &result) {
    return false;
}

bool redis_hash::hgetall(redis_coder &result) {
    return false;
}

} // namespace pkv
