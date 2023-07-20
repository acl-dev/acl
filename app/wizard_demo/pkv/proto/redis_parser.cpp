//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#include "stdafx.h"
#include "redis_parser.h"

namespace pkv {

redis_parser::redis_parser() {
    curr_   = nullptr;
    dbuf_   = new (1) acl::dbuf_pool();
    curr_ = std::make_shared<redis_object>(dbuf_);
}

redis_parser::~redis_parser() {
    dbuf_->destroy();
}

const char* redis_parser::update(const char* data, size_t& len) {
    while (len > 0) {
        data = curr_->update(data, len);
        if (curr_->finish()) {
            objs_.emplace_back(curr_);
            curr_ = std::make_shared<redis_object>(dbuf_);
        } else if (curr_->failed()) {
            break;
        }
    }

    return data;
}

bool redis_parser::to_string(acl::string& out) const {
    for (const auto& obj : objs_) {
        if (!obj->to_string(out)) {
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////

bool test_redis_parse(const char* filepath) {
    acl::string buf;
    if (!acl::ifstream::load(filepath, buf)) {
        printf("load %s error %s\r\n", filepath, acl::last_serror());
        return false;
    }

    redis_parser parser;
    const char* data = buf.c_str();
    size_t len = buf.size();
    const char* left = parser.update(data, len);

    if (len > 0) {
        printf(">>>parse failed<<<\r\n");
        printf("%s\r\n", left);
        return false;
    }

    printf(">>>parse success<<<\r\n");

    acl::string out;

    if (!parser.to_string(out)) {
        printf(">>>build failed<<<\r\n");
        return false;
    }

    if (out != buf) {
        printf(">>>build failed<<<\r\n");
        printf("output:\r\n|%s|\r\n", out.c_str());
        printf("input:\r\n|%s|\r\n", buf.c_str());

        acl::string filetmp(filepath);
        filetmp += ".tmp";
        acl::ofstream fp;
        if (fp.open_trunc(filetmp)) {
            fp.write(out);
        }

        return false;
    }

    printf(">>>build successfully<<<\r\n");
    printf("%s\r\n", out.c_str());

    return true;
}

} // namespace pkv