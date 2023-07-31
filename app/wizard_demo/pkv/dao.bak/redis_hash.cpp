//
// Created by shuxin ¡¡¡¡zheng on 2023/7/21.
//

#include "stdafx.h"
#include "redis_hash.h"

namespace pkv {

redis_hash::redis_hash() {}

redis_hash::~redis_hash() {}

bool redis_hash::update(const char* cmd, const redis_object& obj) {
    if (cmd == NULL || *cmd == 0) {
        return false;
    }
    cmd_ = cmd;
    return true;
}

} // namespace pkv