//
// Created by zsx on 2023/7/23.
//

#include "stdafx.h"
#include "coder/redis_object.h"
#include "coder/redis_coder.h"
#include "redis_key.h"
#include "redis_string.h"
#include "redis_hash.h"
#include "redis_server.h"
#include "redis_handler.h"

namespace pkv {

#define EQ  !strcasecmp

redis_handler::redis_handler(shared_db& db, redis_coder& parser,
     acl::socket_stream& conn)
: db_(db)
, parser_(parser)
, conn_(conn)
, builder_(parser.get_cache())
, coder_(parser.get_cache())
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

    //printf(">>>%s(%d): cmd=%s\r\n", __func__, __LINE__, cmd);

    if (EQ(cmd, "SET")) {
        redis_string redis(*this, obj);
        return redis.set(builder_);
    } else if (EQ(cmd, "GET")) {
        redis_string redis(*this, obj);
        return redis.get(builder_);
    } else if (EQ(cmd, "DEL")) {
        redis_key redis(*this, obj);
        return redis.del(builder_);
    } else if (EQ(cmd, "TYPE")) {
        redis_key redis(*this, obj);
        return redis.type(builder_);
    } else if (EQ(cmd, "HSET")) {
        redis_hash redis(*this, obj);
        return redis.hset(builder_);
    } else if (EQ(cmd, "HGET")) {
        redis_hash redis(*this, obj);
        return redis.hget(builder_);
    } else if (EQ(cmd, "HDEL")) {
        redis_hash redis(*this, obj);
        return redis.hdel(builder_);
    } else if (EQ(cmd, "HMSET")) {
        redis_hash redis(*this, obj);
        return redis.hmset(builder_);
    } else if (EQ(cmd, "HMGET")) {
        redis_hash redis(*this, obj);
        return redis.hmget(builder_);
    } else if (EQ(cmd, "HGETALL")) {
        redis_hash redis(*this, obj);
        return redis.hgetall(builder_);
    } else if (EQ(cmd, "CONFIG")) {
        redis_server redis(*this, obj);
        return redis.config(builder_);
    }

    std::string err;
    err.append(cmd).append("not support yet");
    logger_error("cmd=%s not support!", cmd);
    builder_.create_object().set_error(err);
    return true;
}

} // namespace pkv
