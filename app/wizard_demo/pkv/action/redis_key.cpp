//
// Created by shuxin ¡¡¡¡zheng on 2023/7/31.
//

#include "stdafx.h"
#include "proto/redis_coder.h"
#include "dao/db.h"

#include "redis_handler.h"
#include "redis_key.h"

namespace pkv {

redis_key::redis_key(redis_handler& handler, const redis_object& obj)
: handler_(handler), obj_(obj)
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
    auto& coder = handler_.get_coder();
    coder.clear();

    std::string buff;
    result.create_object().set_status("string");
    return true;
}

} // namespace pkv
