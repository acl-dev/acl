//
// Created by shuxin ¡¡¡¡zheng on 2023/8/1.
//

#include "stdafx.h"
#include "coder/redis_coder.h"
#include "coder/redis_object.h"
#include "dao/hash.h"

#include "redis_handler.h"
#include "redis_hash.h"

namespace pkv {

redis_hash::redis_hash(redis_handler &handler, const redis_object &obj)
: redis_command(handler, obj)
{}

bool redis_hash::hset(redis_coder &result) {
    if (obj_.size() < 4) {
        logger_error("invalid HSET command's size=%zd < 4", obj_.size());
        return false;
    }

    auto key = obj_[1];
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    auto name = obj_[2];
    if (name == nullptr || *name == 0) {
        logger_error("name null");
        return false;
    }

    auto value = obj_[3];
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
    if (obj_.size() < 3) {
        logger_error("invalid HGET command's size=%zd < 3", obj_.size());
        return false;
    }

    auto key = obj_[1];
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    auto name = obj_[2];
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
    if (obj_.size() < 3) {
        logger_error("invalid HDEL params' size=%zd < 3", obj_.size());
        return false;
    }

    auto key = obj_[1];
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    auto name = obj_[2];
    if (name == nullptr || *name == 0) {
        logger_error("name null");
        return false;
    }

    dao::hash dao;
    int ret = dao.hdel(handler_.get_db(), key, name);
    result.create_object().set_number(ret);
    return true;
}

bool redis_hash::hmset(redis_coder &result) {
    return false;
}

bool redis_hash::hmget(redis_coder &result) {
    return false;
}

bool redis_hash::hgetall(redis_coder &result) {
    if (obj_.size() < 2) {
        logger_error("invalid HGETALL command's size=%zd < 2", obj_.size());
        return false;
    }

    auto key = obj_[1];
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    dao::hash dao;
    if (!dao.hgetall(handler_.get_db(), key)) {
        logger_error("dao.hgetall error, key=%s", key);
        return false;
    }

    auto& fields = dao.get_fields();
    auto& obj = result.create_object();
    for (const auto& it : fields) {
        obj.create_child().set_string(it.first, true)
            .create_child().set_string(it.second);
    }
    return true;
}

} // namespace pkv
