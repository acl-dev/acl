//
// Created by zsx on 2023/8/3.
//

#include "stdafx.h"
#include "redis_object.h"
#include "redis_ocache.h"

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

redis_ocache::redis_ocache(size_t max) : max_(max) {
#ifndef USE_VECTOR
    cache_ = new redis_object* [max];
    for (size_t i = 0; i < max; i++) {
        cache_[i] = NULL;
    }
    pos_ = 0;
#endif
}

redis_ocache::~redis_ocache() {
#ifdef USE_VECTOR
    for (auto o : cache_) {
        o->destroy();
    }
#else
    for (size_t i = 0; i < pos_; i++) {
        cache_[i]->destroy();
    }
    delete [] cache_;
#endif
}

redis_object *redis_ocache::get() {
#ifdef USE_VECTOR
    if (UNLIKELY(EMPTY(cache_))) {
        return new redis_object(*this);
    } else {
        auto obj = cache_.back();
        cache_.pop_back();
        return obj;
    }
#else
    if (LIKELY(pos_ > 0)) {
        return cache_[--pos_];
    } else {
        return new redis_object(*this);
    }
#endif
}

void redis_ocache::put(redis_object* obj) {
#ifdef USE_VECTOR
    if (cache_.size() < max_) {
        cache_.push_back(obj);
        obj->reset();
    } else {
        obj->destroy();
    }
#else
    if (LIKELY(pos_ < max_)) {
        assert(obj);
        cache_[pos_++] = obj;
    	obj->reset();
    } else {
        obj->destroy();
    }
#endif
}

} // namespace pkv
