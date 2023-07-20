//
// Created by shuxin 　　zheng on 2023/7/19.
//

#include "stdafx.h"
#include "redis_type.h"
#include "redis_object.h"

namespace pkv {

redis_object::redis_object(acl::dbuf_pool* dbuf, redis_object* parent) {
    dbuf_   = dbuf;
    parent_ = parent;
    status_ = redis_s_begin;
    cnt_    = 0;
    rr_     = nullptr;
    obj_    = nullptr;
}

struct status_machine {
    /* 状态码 */
    int status;

    /* 状态机处理函数 */
    const char* (redis_object::*func) (const char*, size_t&);
};

static struct status_machine status_tab[] = {
    { redis_s_null,     &redis_object::parse_null       },
    { redis_s_begin,    &redis_object::parse_begin      },
    { redis_s_status,   &redis_object::parse_status     },
    { redis_s_error,    &redis_object::parse_error      },
    { redis_s_number,   &redis_object::parse_number     },
    { redis_s_strlen,   &redis_object::parse_strlen     },
    { redis_s_string,   &redis_object::parse_string     },
    { redis_s_strend,   &redis_object::parse_strend     },
    { redis_s_arlen,    &redis_object::parse_arlen      },
    { redis_s_array,    &redis_object::parse_array      },
};

const char* redis_object::update(const char *data, size_t& len) {
    if (failed() || finish())  {
        return data;
    }

    if (obj_) {
        return parse_object(data, len);
    }

    while (len > 0) {
        data  = (this->*(status_tab[status_].func))(data, len);
        if (status_ == redis_s_null || status_ == redis_s_finish) {
            break;
        }
    }

    return data;
}

const char* redis_object::parse_object(const char* data, size_t& len) {
    assert(cnt_ > 0);
    assert(obj_);

    data = obj_->update(data, len);
    if (obj_->failed()) {
        status_ = redis_s_null;
        return data;
    }

    if (!obj_->finish()) {
        return data;
    }

    objs_.emplace_back(obj_);

    if (objs_.size() == (size_t) cnt_) {
        obj_ = nullptr;
        cnt_ = 0;
        status_ = redis_s_finish;
    } else {
        obj_ = std::make_shared<redis_object>(dbuf_, this);
    }
    return data;
}

const char* redis_object::parse_null(const char*, size_t&) {
    logger_error("invalid redis data");
    return nullptr;
}

const char* redis_object::parse_begin(const char* data, size_t& len) {
    if (len == 0) {
        return data;
    }

    switch (*data) {
    case ':':	// INTEGER
        status_ = redis_s_number;
        break;
    case '+':	// STATUS
        status_ = redis_s_status;
        break;
    case '-':	// ERROR
        status_ = redis_s_error;
        break;
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

    len--;
    data++;
    return data;
}

const char* redis_object::parse_status(const char* data, size_t& len) {
    bool found = false;
    data = get_line(data, len, found);
    if (!found) {
        assert(len == 0);
        return data;
    }

    if (buf_.empty()) {
        status_ = redis_s_null;
        return data;
    }

    rr_ = new(dbuf_) acl::redis_result(dbuf_);
    rr_->set_type(acl::REDIS_RESULT_STATUS);
    rr_->set_size(1);
    put_data(dbuf_, rr_, buf_.c_str(), buf_.length());
    buf_.clear();

    status_ = redis_s_finish;
    return data;
}

const char* redis_object::parse_error(const char* data, size_t& len) {
    bool found = false;
    data = get_line(data, len, found);
    if (!found) {
        assert(len == 0);
        return data;
    }

    if (buf_.empty()) {
        status_ = redis_s_null;
        return data;
    }

    rr_ = new(dbuf_) acl::redis_result(dbuf_);
    rr_->set_type(acl::REDIS_RESULT_ERROR);
    rr_->set_size(1);
    put_data(dbuf_, rr_, buf_.c_str(), buf_.length());
    buf_.clear();

    status_ = redis_s_finish;
    return data;
}

const char* redis_object::parse_number(const char* data, size_t& len) {
    bool found = false;
    data = get_line(data, len, found);
    if (!found) {
        assert(len == 0);
        return data;
    }

    if (buf_.empty()) {
        status_ = redis_s_null;
        return data;
    }

    rr_ = new(dbuf_) acl::redis_result(dbuf_);
    rr_->set_type(acl::REDIS_RESULT_INTEGER);
    rr_->set_size(1);
    put_data(dbuf_, rr_, buf_.c_str(), buf_.length());
    buf_.clear();

    status_ = redis_s_finish;
    return data;
}

const char* redis_object::parse_strlen(const char* data, size_t& len) {
    cnt_ = 0;
    data = get_length(data, len, cnt_);
    if (status_ == redis_s_null) {
        return data;
    }

    rr_ = new(dbuf_) acl::redis_result(dbuf_);
    rr_->set_type(acl::REDIS_RESULT_STRING);

    if (cnt_ <= 0) {
        status_ = redis_s_finish;
        return data;
    }

    rr_->set_size((size_t) cnt_);
    buf_.clear();

    status_ = redis_s_string;
    return data;
}

const char* redis_object::parse_string(const char* data, size_t& len) {
    assert (rr_ != nullptr);

    data = get_data(data, len, (size_t) cnt_);
    if (buf_.size() == (size_t) cnt_) {
        status_ = redis_s_strend;
        put_data(dbuf_, rr_, buf_.c_str(), buf_.length());
        buf_.clear();
        cnt_ = 0;
    }

    return data;
}

const char* redis_object::parse_strend(const char* data, size_t& len) {
    assert (rr_ != nullptr);

    bool found = false;
    data = get_line(data, len, found);

    // If the buf_ not empty, some data other '\r\n' got.
    if (!buf_.empty()) {
        status_ = redis_s_null;
        return data;
    }

    if (!found) {
        assert(len == 0);
        return data;
    }

    status_ = redis_s_finish;
    return data;
}

const char* redis_object::parse_arlen(const char* data, size_t& len) {
    cnt_ = 0;
    data = get_length(data, len, cnt_);
    if (status_ == redis_s_null) {
        return data;
    }

    rr_ = new(dbuf_) acl::redis_result(dbuf_);
    rr_->set_type(acl::REDIS_RESULT_ARRAY);

    if (cnt_ <= 0) {
        status_ = redis_s_finish;
        return data;
    }

    rr_->set_size((size_t) cnt_);

    buf_.clear();
    status_ = redis_s_array;
    obj_ = std::make_shared<redis_object>(dbuf_, this);
    return data;
}

const char* redis_object::parse_array(const char* data, size_t& len) {
    assert(rr_ != nullptr);
    assert(obj_ != nullptr);

    return parse_object(data, len);
}

const char* redis_object::get_data(const char* data, size_t& len, size_t want) {
    size_t n = buf_.size();
    assert(n < want);

    want -= n;
    for (size_t i = 0; i < want && len > 0; i++) {
        buf_.push_back(*data++);
        len--;
    }
    return data;
}

const char* redis_object::get_length(const char* data, size_t& len, int& res) {
    bool found = false;
    data = get_line(data, len, found);
    if (!found) {
        assert(len == 0);
        return data;
    }

    if (buf_.empty()) {
        status_ = redis_s_null;
        return data;
    }

    res = atoi(buf_.c_str());
    buf_.clear();
    return data;
}

const char* redis_object::get_line(const char* data, size_t& len, bool& found) {
    while (len > 0) {
        switch (*data) {
        case '\r':
            data++;
            len--;
            break;
        case '\n':
            data++;
            len--;
            found = true;
            return data;
        default:
            buf_.push_back(*data++);
            len--;
            break;
        }
    }
    return data;
}

void redis_object::put_data(acl::dbuf_pool* dbuf, acl::redis_result* rr,
      const char* data, size_t len) {
    char* buf = (char*) dbuf->dbuf_alloc(len + 1);
    if (len > 0) {
        memcpy(buf, data, len);
    }

    buf[len] = 0;
    rr->put(buf, len);
}

bool redis_object::to_string(acl::string& out) const {
#define USE_UNIX_CRLF
#ifdef USE_UNIX_CRLF
#define CRLF    "\n"
#else
#define CRLF    "\r\n"
#endif

    if (!objs_.empty()) {
        out.format_append("*%zd%s", objs_.size(), CRLF);

        for (auto obj : objs_) {
            if (!obj->to_string(out)) {
                return false;
            }
        }
    }

    if (rr_ == nullptr) {
        return false;
    }

    switch (rr_->get_type()) {
    case acl::REDIS_RESULT_STATUS:
        out.format_append("+%s%s", rr_->get_status(), CRLF);
        break;
    case acl::REDIS_RESULT_ERROR:
        out.format_append("-%s%s", rr_->get_error(), CRLF);
        break;
    case acl::REDIS_RESULT_INTEGER:
        out.format_append(":%lld%s", rr_->get_integer64(), CRLF);
        break;
    case acl::REDIS_RESULT_STRING:
        out.format_append("$%zd%s", rr_->get_length(), CRLF);
        rr_->argv_to_string(out, false);
        out += CRLF;
        break;
    case acl::REDIS_RESULT_ARRAY:
        break;
    default:
        break;
    }
    return true;
}

} // namespace pkv
