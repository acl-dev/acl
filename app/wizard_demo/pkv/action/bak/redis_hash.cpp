//
// Created by shuxin ¡¡¡¡zheng on 2023/8/1.
//

#include "stdafx.h"
#include "proto/redis_coder.h"
#include "proto/redis_object.h"

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

    auto& coder = handler_.get_coder();
    coder.clear();

    coder.create_object()
        .create_child().set_string(name, true)
        .create_child().set_string(value, true);

    std::string buff;
    if (!coder.to_string(buff)) {
        logger_error("build data error");
        return false;
    }

    if (!handler_.get_db()->set(key, buff)) {
        logger_error("set key=%s, value=%s error", key, buff.c_str());
        return false;
    }

    //printf(">set key=%s, value=%s ok\n", key, buff.c_str());
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
    if (!handler_.get_db()->get(key, buff) || buff.empty()) {
        logger_error("db get key=%s error", key);
        return false;
    }

    //printf(">>hget: [%s]\r\n", buff.c_str());

    auto& coder = handler_.get_coder();
    coder.clear();

    size_t len = buff.size();
    (void) coder.update(buff.c_str(), len);
    if (len > 0) {
        logger_error("invalid buff in db for key=%s", key);
        return false;
    }

    auto& objs2 = coder.get_objects();
    if (objs2.size() != 1) {
        logger_error("invalid object in db, key=%s, objs=%zd", key, objs2.size());
        return false;
    }

    auto array = objs2[0];
    if (array->get_type() != REDIS_OBJ_ARRAY) {
        logger_error("invalid array object, key=%s", key);
        return false;
    }
    auto& objs3 = array->get_objects();
    if (objs3.empty() || objs3.size() % 2 != 0) {
        logger_error("invalid array objects' size=%zd, key=%s", objs3.size(), key);
        return false;
    }
    for (size_t i = 0; i < objs3.size();) {
        auto n = objs3[i++]->get_str();
        auto v = objs3[i++]->get_str();
        if (n == nullptr || *n == 0 || v == nullptr || *v == 0) {
            logger_error("no value set in db, key=%s", key);
            return false;
        }
        if (strcmp(name, n) == 0) {
            result.create_object().set_string(v);
            return true;
        }
    }

    logger_error("Not found, key=%s, name=%s", key, name);
    return false;
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