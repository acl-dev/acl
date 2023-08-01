//
// Created by shuxin ¡¡¡¡zheng on 2023/7/31.
//

#pragma once

namespace pkv {

class redis_handler;
class redis_object;
class redis_coder;

class redis_string {
public:
    redis_string(redis_handler& handler, const redis_object &obj);
    ~redis_string() = default;

    bool set(redis_coder& result);
    bool get(redis_coder& result);

private:
    redis_handler& handler_;
    const redis_object &obj_;
};

} // namespace pkv
