//
// Created by zsx on 2023/8/3.
//

#pragma once
#include <vector>

namespace pkv {

class redis_object;

class redis_ocache {
public:
    redis_ocache(size_t max = 100);
    ~redis_ocache();

    redis_object* get();
    void put(redis_object* obj);

private:
    size_t max_;
#ifdef USE_VECTOR
    std::vector<redis_object*> cache_;
#else
    redis_object** cache_;
    size_t pos_;
#endif
};

} // namespace pkv
