//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#include "stdafx.h"
#include "redis_coder.h"

namespace pkv {

redis_coder::redis_coder() {
    dbuf_ = new (1) acl::dbuf_pool();
    curr_ = new(dbuf_) redis_object(dbuf_, nullptr);
}

redis_coder::~redis_coder() {
    dbuf_->destroy();
}

void redis_coder::clear() {
    objs_.clear();
}

const char* redis_coder::update(const char* data, size_t& len) {
    while (len > 0) {
        data = curr_->update(data, len);
        if (curr_->finish()) {
            objs_.push_back(curr_);
            curr_ = new(dbuf_) redis_object(dbuf_, nullptr);
        } else if (curr_->failed()) {
            break;
        }
    }

    return data;
}

redis_object& redis_coder::create_object() {
    auto obj = new(dbuf_) redis_object(dbuf_, nullptr);
    objs_.push_back(obj);
    return *obj;
}

bool redis_coder::to_string(acl::string& out) const {
    for (const auto& obj : objs_) {
        if (!obj->to_string(out)) {
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////
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

    redis_coder parser;
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

    redis_coder parser;
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
    const char* cmd = parser.get_objects()[0]->get_cmd();
    printf(">>%s(%d): build successfully, cmd=%s<<\r\n",
           __func__, __LINE__, cmd ? cmd : "unknown");
    return true;
}

//////////////////////////////////////////////////////////////////////////////

bool test_redis_build() {
    redis_coder builder;

    auto& obj = builder.create_object();

#if 0
    obj.create_child().set_string("HMSET");
    obj.create_child().set_string("hash-key");
    obj.create_child().set_string("field1");
    obj.create_child().set_string("vaule1");
    obj.create_child().set_string("field2");
    obj.create_child().set_string("value2");
    obj.create_child().set_string("field3");
    obj.create_child().set_string("value3");
#else
    obj.create_child().set_string("HMSET", true)
        .create_child().set_string("hash-key", true)
        .create_child().set_string("field1", true)
        .create_child().set_string("value1", true)
        .create_child().set_string("field2", true)
        .create_child().set_string("value2", true)
        .create_child().set_string("field3", true)
        .create_child().set_string("value3", true);
#endif

    acl::string buf;
    if (builder.to_string(buf)) {
        const char* cmd = obj.get_cmd();
        printf("%s(%d): build redis successfully, cmd=%s\r\n",
               __func__, __LINE__, cmd ? cmd : "unknown");
        printf("[%s]\r\n", buf.c_str());
    } else {
        printf("%s(%d): build redis failed\r\n", __func__, __LINE__);
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
} // namespace pkv