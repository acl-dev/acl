//
// Created by shuxin ¡¡¡¡zheng on 2023/7/31.
//

#include "stdafx.h"
#include "redis_key.h"

namespace pkv {

redis_key::redis_key(shared_db& db, const redis_object& obj, redis_coder& base)
: db_(db), obj_(obj), base_(base)
{
    (void) base_;
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

    if (!db_->del(key)) {
        logger_error("db del error, key=%s", key);
        return false;
    }

    result.create_object().set_number(1);
    return true;
}

bool redis_key::type(redis_coder& result) {
    result.create_object().set_status("string");
    return true;
}

} // namespace pkv
