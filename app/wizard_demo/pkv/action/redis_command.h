//
// Created by shuxin ¡¡¡¡zheng on 2023/8/1.
//

#pragma once

namespace pkv {

class redis_handler;
class redis_object;

class redis_command {
public:
    redis_command(redis_handler& handler, const redis_object& obj);
    virtual ~redis_command() = default;

protected:
    redis_handler& handler_;
    const redis_object &obj_;
};

} // namespace pkv
