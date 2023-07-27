//
// Created by zsx on 2023/7/23.
//

#include "stdafx.h"
#include "proto/redis_object.h"
#include "proto/redis_coder.h"
#include "redis_handler.h"

namespace pkv {

#define EQ  !strcasecmp

redis_handler::redis_handler(shared_db& db, const redis_coder& parser,
     acl::socket_stream& conn)
: db_(db)
, parser_(parser)
, conn_(conn)
{
}

bool redis_handler::handle() {
    auto objs = parser_.get_objects();
    if (objs.empty()) {
        return true;
    }

#if 0
    {
	std::string tmp;
	for (size_t i = 0; i < objs.size(); i++) {
		tmp += "+OK\r\n";
	}
	return conn_.write(tmp.c_str(), tmp.size()) == (int) tmp.size();
    }
#endif

    //if (objs.size() >= 20) { printf(">>>objs=%zd\r\n", objs.size()); }

    for (const auto& obj : objs) {
        if (!handle_one(*obj)) {
            return false;
        }
    }

    std::string buf;
    if (!builder_.to_string(buf)) {
        builder_.clear();
        return false;
    }

    builder_.clear();

    //if (objs.size() >= 20) { printf("reply len=%zd\r\n", buf.size()); }

    //printf(">>>buf=[%s]\r\n", buf.c_str());
    return conn_.write(buf.c_str(), buf.size()) != -1;
}

bool redis_handler::handle_one(const redis_object &obj) {
    auto cmd = obj.get_cmd();
    if (cmd == nullptr || *cmd == '\0') {
        logger_error("redis command null");
        return false;
    }

    //printf(">>>cmd=%s\r\n", cmd);
    //
    //
    if (EQ(cmd, "SET")) {
        return set(obj);
    } else if (EQ(cmd, "GET")) {
        return get(obj);
    } else if (EQ(cmd, "DEL")) {
        return del(obj);
    } else if (EQ(cmd, "TYPE")) {
        return type(obj);
    } else if (EQ(cmd, "HSET")) {
        return hset(obj);
    } else if (EQ(cmd, "HGET")) {
        return hget(obj);
    } else if (EQ(cmd, "HDEL")) {
        return hdel(obj);
    } else if (EQ(cmd, "HMSET")) {
        return hmset(obj);
    } else if (EQ(cmd, "HMGET")) {
        return hmget(obj);
    } else if (EQ(cmd, "HGETALL")) {
        return hgetall(obj);
    }

    std::string err;
    err.append(cmd).append("not support yet");
    logger_error("cmd=%s not support!", cmd);
    builder_.create_object().set_error(err);
    return true;
}

bool redis_handler::set(const redis_object &obj) {
    auto& objs = obj.get_objects();
    if (objs.size() < 3) {
        logger_error("invalid SET params' size=%zd", objs.size());
        return false;
    }

    auto key = objs[1]->get_str();
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    auto value = objs[2]->get_str();
    if (value == nullptr || *value == 0) {
        logger_error("value null");
        return false;
    }

    if (!var_cfg_disable_serialize) {
        std::string buff;
        coder_.create_object().set_string(value);
        coder_.to_string(buff);
        coder_.clear();

        if (!var_cfg_disable_save) {
            if (!db_->set(key, buff.c_str())) {
                logger_error("db set error, key=%s", key);
                return false;
            }
        }
    }

    builder_.create_object().set_status("OK");
    return true;
}

bool redis_handler::get(const redis_object &obj) {
    auto& objs = obj.get_objects();
    if (objs.size() < 2) {
        logger_error("invalid GET params' size=%zd", objs.size());
        return false;
    }

    auto key = objs[1]->get_str();
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    std::string buff;
    if (!db_->get(key, buff) || buff.empty()) {
        logger_error("db get error, key=%s", key);
        return false;
    }

    redis_coder builder;
    size_t len = buff.size();
    (void) builder.update(buff.c_str(), len);
    if (len > 0) {
        logger_error("invalid buff in db, key=%s", key);
        return false;
    }

    auto& objs2 = builder.get_objects();
    if (objs2.size() != 1) {
        logger_error("invalid object in db, key=%s, size=%zd", key, objs2.size());
        return false;
    }

    auto o = objs2[0];
    if (o->get_type() != REDIS_OBJ_STRING) {
        logger_error("invalid object type=%d, key=%s", (int) o->get_type(), key);
        return false;
    }

    auto v = o->get_str();
    if (v == nullptr || *v == 0) {
        logger_error("value null, key=%s", key);
        return false;
    }
    builder_.create_object().set_string(v);
    return true;
}

bool redis_handler::del(const redis_object &obj) {
    auto& objs = obj.get_objects();
    if (objs.size() < 2) {
        logger_error("invalid SET params' size=%zd", objs.size());
        return false;
    }

    auto key = objs[1]->get_str();
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    if (!db_->del(key)) {
        logger_error("db del error, key=%s", key);
        return false;
    }

    builder_.create_object().set_number(1);
    return true;
}

bool redis_handler::type(const redis_object &obj) {
    builder_.create_object().set_status("string");
    return true;
}

bool redis_handler::hset(const redis_object &obj) {
    auto& objs = obj.get_objects();
    if (objs.size() < 4) {
        logger_error("invalid HSET command's size=%zd < 4", objs.size());
        return false;
    }

    auto key = objs[1]->get_str();
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    auto name = objs[2]->get_str();
    if (name == nullptr || *name == 0) {
        logger_error("name null");
        return false;
    }

    auto value = objs[3]->get_str();
    if (value == nullptr || *value == 0) {
        logger_error("value null");
        return false;
    }

    redis_coder builder;
    builder.create_object()
        .create_child().set_string(name, true)
        .create_child().set_string(value, true);

    std::string buff;
    if (!builder.to_string(buff)) {
        logger_error("build data error");
        return false;
    }

    if (!db_->set(key, buff)) {
        logger_error("set key=%s, value=%s error", key, buff.c_str());
        return false;
    }

    //printf(">set key=%s, value=%s ok\n", key, buff.c_str());
    builder_.create_object().set_number(1);
    return true;
}

bool redis_handler::hget(const redis_object &obj) {
    auto& objs = obj.get_objects();
    if (objs.size() < 3) {
        logger_error("invalid HGET command's size=%zd < 3", objs.size());
        return false;
    }

    auto key = objs[1]->get_str();
    if (key == nullptr || *key == 0) {
        logger_error("key null");
        return false;
    }

    auto name = objs[2]->get_str();
    if (name == nullptr || *name == 0) {
        logger_error("name null");
        return false;
    }

    std::string buff;
    if (!db_->get(key, buff) || buff.empty()) {
        logger_error("db get key=%s error", key);
        return false;
    }

    //printf(">>hget: [%s]\r\n", buff.c_str());

    redis_coder builder;
    size_t len = buff.size();
    (void) builder.update(buff.c_str(), len);
    if (len > 0) {
        logger_error("invalid buff in db for key=%s", key);
        return false;
    }

    auto& objs2 = builder.get_objects();
    if (objs2.size() != 1) {
        logger_error("invalid object in db, key=%s, objs=%zd", key, objs2.size());
        return false;
    }

    auto array = objs2[0];
    if (array->get_type() != REDIS_OBJ_ARRAY) {
        logger_error("invalid array object, key=%s", key);
        return false;
    }
    auto& objs3 = array->get_objects();
    if (objs3.empty() || objs3.size() % 2 != 0) {
        logger_error("invalid array objects' size=%zd, key=%s", objs3.size(), key);
        return false;
    }
    for (size_t i = 0; i < objs3.size();) {
        auto n = objs3[i++]->get_str();
        auto v = objs3[i++]->get_str();
        if (n == nullptr || *n == 0 || v == nullptr || *v == 0) {
            logger_error("no value set in db, key=%s", key);
            return false;
        }
        if (strcmp(name, n) == 0) {
            builder_.create_object().set_string(v);
            return true;
        }
    }

    logger_error("Not found, key=%s, name=%s", key, name);
    return false;
}

bool redis_handler::hdel(const redis_object &obj) {

    return true;
}

bool redis_handler::hmset(const redis_object &obj) {

    return true;
}

bool redis_handler::hmget(const redis_object &obj) {

    return true;
}

bool redis_handler::hgetall(const redis_object &obj) {

    return true;
}

} // namespace pkv
