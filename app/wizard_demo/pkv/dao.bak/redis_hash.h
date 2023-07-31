//
// Created by shuxin ¡¡¡¡zheng on 2023/7/21.
//

#pragma once

namespace pkv {

class redis_object;

class redis_hash {
public:
    redis_hash();
    ~redis_hash();

    bool update(const char* cmd, const redis_object& obj);

private:
    std::string cmd_;
    std::string key_;
    std::map<std::string, std::string> fields_;
};

} // namespace pkv