//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#include "stdafx.h"
#include "redis_type.h"
#include "redis_parser.h"

namespace pkv {

redis_parser::redis_parser() {
    status_ = redis_s_obj;
    curr_   = nullptr;
}

size_t redis_parser::update(const char *data, size_t len) {
    if (curr_ == NULL) {
        curr_ = std::make_shared<redis_object>();
    }

    while (len > 0) {
        len = curr_->update(data, len);
        if (curr_->finish()) {
            objs_.emplace_back(curr_);
            curr_ = std::make_shared<redis_object>();
        }
    }

    return len;
}

} // namespace pkv