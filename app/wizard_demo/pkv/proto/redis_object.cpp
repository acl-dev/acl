//
// Created by shuxin 　　zheng on 2023/7/19.
//

#include "stdafx.h"
#include "redis_type.h"
#include "redis_object.h"

namespace pkv {

struct status_machine {
    /* 状态码 */
    int status;

    /* 状态机处理函数 */
    size_t (redis_object::*func) (const char*, size_t);
};

static struct status_machine status_tab[] = {
    { redis_s_null,     &redis_object::parse_null       },
    { redis_s_obj,      &redis_object::parse_obj        },
    { redis_s_arlen,    &redis_object::parse_arlen      },
    { redis_s_array,    &redis_object::parse_array      },
    { redis_s_number,   &redis_object::parse_number     },
    { redis_s_strlen,   &redis_object::parse_strlen     },
    { redis_s_string,   &redis_object::parse_string     },
    { redis_s_status,   &redis_object::parse_status     },
    { redis_s_error,    &redis_object::parse_error      },
};

redis_object::redis_object() {
    finish_ = false;
    status_ = redis_s_obj;
    rr_ = nullptr;
    obj_ = nullptr;
    dbuf_ = new (1) acl::dbuf_pool();
}

redis_object::~redis_object() {
    dbuf_->destroy();
}

size_t redis_object::update(const char *data, size_t len) {
    while (len > 0) {
        len  = (this->*(status_tab[status_].func))(data, len);
    }

    return len;
}

size_t redis_object::parse_null(const char*, size_t) {
    logger_error("invalid redis data");
    return 0;
}

size_t redis_object::parse_obj(const char* data, size_t len) {
    if (len == 0) {
        return 0;
    }

    switch (*data) {
    case '-':	// ERROR
        status_ = redis_s_error;
        break;
    case '+':	// STATUS
        status_ = redis_s_status;
        break;
    case ':':	// INTEGER
        status_ = redis_s_number;
    case '$':	// STRING
        status_ = redis_s_strlen;
        break;
    case '*':	// ARRAY
        status_ = redis_s_arlen;
        break;
    default:	// INVALID
        status_ = redis_s_null;
        break;
    }
    return len;
}

size_t redis_object::parse_number(const char* data, size_t len) {
    bool found = false;
    len = get_line(data, len, found);
    if (!found) {
        assert(len == 0);
        return len;
    }

    if (buf_.empty()) {
        status_ = redis_s_null;
        return len;
    }

    acl::redis_result* rr = new(dbuf_) acl::redis_result(dbuf_);
    rr->set_type(acl::REDIS_RESULT_INTEGER);
    rr->set_size(1);
    put_data(dbuf_, rr, buf_.c_str(), buf_.length());
    buf_.clear();

    status_ = redis_s_obj;
    return len;
}

size_t redis_object::parse_strlen(const char* data, size_t len) {
    len_ = 0;
    len = get_length(data, len, len_);
    if (status_ == redis_s_null) {
        return 0;
    }

    rr_ = new(dbuf_) acl::redis_result(dbuf_);
    rr_->set_type(acl::REDIS_RESULT_STRING);

    if (len_ <= 0) {
        status_ = redis_s_obj;
        return len;
    }

    rr_->set_size((size_t) len_);
    buf_.clear();
    status_ = redis_s_string;
    return len;
}

size_t redis_object::parse_string(const char* data, size_t len) {
    if (rr_ == nullptr) {
        status_ = redis_s_null;
        return 0;
    }

    len = get_data(data, len, (size_t) len_);
    if (buf_.size() == (size_t) len_) {
        status_ = redis_s_obj;
    }
    return len;
}

size_t redis_object::parse_arlen(const char* data, size_t len) {
    len_ = 0;
    len = get_length(data, len, len_);
    if (status_ == redis_s_null) {
        return 0;
    }

    rr_ = new(dbuf_) acl::redis_result(dbuf_);
    rr_->set_type(acl::REDIS_RESULT_ARRAY);

    if (len_ <= 0) {
        status_ = redis_s_obj;
        return len;
    }

    rr_->set_size((size_t) len_);
    buf_.clear();
    status_ = redis_s_array;
    return len;
}

size_t redis_object::parse_array(const char* data, size_t len) {
    if (rr_ == nullptr) {
        status_ = redis_s_null;
        return 0;
    }

    if (obj_ == nullptr) {
        obj_ = std::make_shared<redis_object>();
    }

    len = get_object(data, len, obj_);
    if (obj_->finish()) {
        objs_.emplace_back(obj_);

        if (objs_.size() == (size_t) len_) {
            obj_ = nullptr;
            status_ = redis_s_obj;
        } else {
            obj_ = std::make_shared<redis_object>();
        }
    }

    return len;
}

size_t redis_object::parse_status(const char* data, size_t len) {
    bool found = false;
    len = get_line(data, len, found);
    if (!found) {
        assert(len == 0);
        return len;
    }

    if (buf_.empty()) {
        status_ = redis_s_null;
        return len;
    }

    acl::redis_result* rr = new(dbuf_) acl::redis_result(dbuf_);
    rr->set_type(acl::REDIS_RESULT_STATUS);
    rr->set_size(1);
    put_data(dbuf_, rr, buf_.c_str(), buf_.length());
    buf_.clear();

    status_ = redis_s_obj;
    return len;
}

size_t redis_object::parse_error(const char* data, size_t len) {
    bool found = false;
    len = get_line(data, len, found);
    if (!found) {
        assert(len == 0);
        return len;
    }

    if (buf_.empty()) {
        status_ = redis_s_null;
        return len;
    }

    acl::redis_result* rr = new(dbuf_) acl::redis_result(dbuf_);
    rr->set_type(acl::REDIS_RESULT_ERROR);
    rr->set_size(1);
    put_data(dbuf_, rr, buf_.c_str(), buf_.length());
    buf_.clear();

    status_ = redis_s_obj;
    return len;
}

size_t redis_object::get_object(const char* data, size_t len, shared_redis obj) {
    len = obj->update(data, len);
    if (obj->get_status() == redis_s_null) {
        status_ = redis_s_null;
        return 0;
    }
    return len;
}

size_t redis_object::get_data(const char* data, size_t len, size_t want) {
    size_t n = buf_.size();
    assert(n < want);

    want -= n;
    for (size_t i = 0; i < want && len > 0; i++) {
        buf_.push_back(*data++);
        len--;
    }
    return len;
}

size_t redis_object::get_length(const char* data, size_t len, int& res) {
    bool found = false;
    len = get_line(data, len, found);
    if (!found) {
        assert(len == 0);
        return len;
    }

    if (buf_.empty()) {
        status_ = redis_s_null;
        return len;
    }

    res = atoi(buf_.c_str());
    buf_.clear();
    return len;
}

size_t redis_object::get_line(const char* data, size_t len, bool& found) {
    while (len > 0) {
        switch (*data) {
        case '\r':
            data++;
            len--;
            break;
        case '\n':
            len--;
            found = true;
            return len;
        default:
            buf_.push_back(*data++);
            len--;
            break;
        }
    }
    return len;
}

void redis_object::put_data(acl::dbuf_pool* dbuf, acl::redis_result* rr,
    const char* data, size_t len)
{
    char* buf = (char*) dbuf->dbuf_alloc(len + 1);
    if (len > 0) {
        memcpy(buf, data, len);
    }

    buf[len] = 0;
    rr->put(buf, len);
}

} // namespace pkv
