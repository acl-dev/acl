//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#include "stdafx.h"
#include "redis_ocache.h"
#include "redis_coder.h"
#include "dao/string.h"

namespace pkv {

#if 1
#define EMPTY(x) ((x).size() == 0)
#else
#define EMPTY(x) ((x).empty())
#endif

#if 0
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#endif

redis_coder::redis_coder(redis_ocache& cache)
: cache_(cache)
{
    curr_ = cache_.get();
}

redis_coder::~redis_coder() {
    clear();
    cache_.put(curr_);
}

void redis_coder::clear() {
    for (auto obj : objs_) {
        cache_.put(obj);
    }
    objs_.clear();
}

const char* redis_coder::update(const char* data, size_t& len) {
    while (len > 0) {
        data = curr_->update(data, len);
        if (curr_->finish()) {
            objs_.push_back(curr_);
            curr_ = cache_.get();
        } else if (curr_->failed()) {
            break;
        }
    }

    return data;
}

redis_object& redis_coder::create_object() {
    redis_object* obj = cache_.get();
    assert(obj);
    objs_.push_back(obj);
    return *obj;
}

bool redis_coder::to_string(std::string& out) const {
    for (const auto& obj : objs_) {
        if (!obj->to_string(out)) {
            return false;
        }
    }

    return true;
}

} // namespace pkv
