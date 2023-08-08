//
// Created by shuxin ¡¡¡¡zheng on 2023/7/31.
//

#include "stdafx.h"
#include "proto/redis_coder.h"

#include "redis_handler.h"
#include "redis_key.h"

namespace pkv {

redis_key::redis_key(redis_handler& handler, const redis_object& obj)
: redis_command(handler, obj)
{
    (void) handler_;
}

bool redis_key::del(redis_coder& result) {
    auto& objs = obj_.get_objects();
    if (objs.size() < 2) {
        logger_error("invalid SET params' size=%zd", objs.size());
        return false;
    }

    auto key = objs[1]->get_str();
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    if (!handler_.get_db()->del(key)) {
        logger_error("db del error, key=%s", key);
        return false;
    }

    result.create_object().set_number(1);
    return true;
}

bool redis_key::type(redis_coder& result) {
    auto& objs = obj_.get_objects();
    if (objs.size() < 2) {
        logger_error("invalid TYPE params' size=%zd", objs.size());
        return false;
    }

    auto key = objs[1]->get_str();
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    std::string buff;
    if (!handler_.get_db()->get(key, buff) || buff.empty()) {
        logger_error("db get error, key=%s", key);
        return false;
    }

    auto& coder = handler_.get_coder();
    coder.clear();

    size_t len = buff.size();
    (void) coder.update(buff.c_str(), len);
    if (len > 0) {
        logger_error("invalid data in db, key=%s, buff=%s, size=%zd, left=%zd",
                key, buff.c_str(), buff.size(), len);
        return false;
    }

    auto& objs2 = coder.get_objects();
    if (objs2.size() != 2) {
        logger_error("invalid object in db, key=%s, size=%zd", key, objs2.size());
        return false;
    }

    auto o = objs2[0];
    if (o->get_type() != REDIS_OBJ_ARRAY) {
        logger_error("invalid object type=%d, key=%s", (int) o->get_type(), key);
        return false;
    }

    auto& objs3 = o->get_objects();
    if (objs3.size()  < 2) {
        logger_error("invalid objects size=%zd", objs3.size());
        return false;
    }

    auto type = objs3[0]->get_str();

    result.create_object().set_status(type);
    return true;
}

} // namespace pkv
