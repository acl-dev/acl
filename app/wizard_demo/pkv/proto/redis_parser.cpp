//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#include "stdafx.h"
#include "redis_parser.h"

namespace pkv {

redis_parser::redis_parser() {
    curr_   = nullptr;
    dbuf_   = new (1) acl::dbuf_pool();
    curr_ = new(dbuf_) redis_object(dbuf_);
}

redis_parser::~redis_parser() {
    dbuf_->destroy();
}

const char* redis_parser::update(const char* data, size_t& len) {
    while (len > 0) {
        data = curr_->update(data, len);
        if (curr_->finish()) {
            objs_.push_back(curr_);
            curr_ = new(dbuf_) redis_object(dbuf_);
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
    if (!test_redis_parse_once(filepath)) {
        return false;
    }

    printf("\r\n");

    if (!test_redis_parse_stream(filepath)) {
        return false;
    }

    return true;
}

bool test_redis_parse_once(const char* filepath) {
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
        printf(">>>%s: parse failed<<<\r\n", __func__);
        printf("%s\r\n", left);
        return false;
    }

    printf(">>>%s: parse success<<<\r\n", __func__);

    acl::string out;

    if (!parser.to_string(out)) {
        printf(">>>%s: build failed<<<\r\n", __func__);
        return false;
    }

    if (out != buf) {
        printf(">>>%s: build failed<<<\r\n", __func__);
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

    printf("%s\r\n", out.c_str());
    printf(">>>%s: build successfully<<<\r\n", __func__);
    return true;
}

bool test_redis_parse_stream(const char* filepath) {
    acl::string buf;

    if (!acl::ifstream::load(filepath, buf)) {
        printf("load %s error %s\r\n", filepath, acl::last_serror());
        return false;
    }

    redis_parser parser;
    const char* data = buf.c_str();
    size_t len = buf.size();
    for (size_t i = 0; i < len; i++) {
        char ch  = *data++;
        size_t n = 1;
        //putchar(ch);
        const char* left = parser.update(&ch, n);
        if (n > 0) {
            printf(">>>%s(%d): parse failed, left=%s<<<\r\n", __func__, __LINE__, left);
            return false;
        }
    }
    printf(">>%s(%d): parse successfully<<<\r\n", __func__, __LINE__);

    acl::string out;

    if (!parser.to_string(out)) {
        printf(">>%s(%d): build failed<<\r\n", __func__, __LINE__);
        return false;
    }

    if (out != buf) {
        printf("%s\r\n", out.c_str());
        printf(">>%s(%d): build failed<<\r\n", __func__, __LINE__);
        return false;
    }

    printf("%s\r\n", out.c_str());
    printf(">>%s(%d): build successfully<<\r\n", __func__, __LINE__);
    return true;
}

} // namespace pkv