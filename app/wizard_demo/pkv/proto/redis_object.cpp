//
// Created by shuxin 　　zheng on 2023/7/19.
//

#include "stdafx.h"
#include "redis_ocache.h"
#include "redis_object.h"

namespace pkv {

#if 1
#define EMPTY(x) ((x).size() == 0)
#else
#define EMPTY(x) ((x).empty())
#endif

#if 0
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#endif

#define CHECK(x) { if ((x) == nullptr) (x) = new std::vector<redis_object*>; }

redis_object::redis_object(redis_ocache& cache)
: parent_(this)
, cache_(cache)
, objs_(NULL)
{
}

redis_object::~redis_object() {
    reset();
    delete objs_;
}

void redis_object::destroy() {
    delete this;
}

void redis_object::set_parent(redis_object* parent) {
    if (parent) {
        parent_ = parent;
    }
}

void redis_object::reset() {
    if (objs_) {
        for (auto obj : *objs_) {
            cache_.put(obj);
        }
        objs_->clear();
    }

    status_ = redis_s_begin;
    type_   = REDIS_OBJ_UNKOWN;
    parent_ = this;
    obj_    = nullptr;
    cnt_    = 0;

    buf_.clear();
}

const char* redis_object::get_cmd() const {
    if (type_ == REDIS_OBJ_STRING) {
        return buf_.c_str();
    }

    if (objs_ == nullptr) {
        return nullptr;
    }

    if (UNLIKELY(EMPTY(*objs_) || type_ != REDIS_OBJ_ARRAY)) {
        return nullptr;
    }

    return (*objs_)[0]->get_cmd();
}

const char* redis_object::get_str() const {
    if (type_ == REDIS_OBJ_STRING) {
        return buf_.c_str();
    }

    return nullptr;
}

const char* redis_object::operator[](size_t i) const {
    if (objs_ == nullptr) {
        return nullptr;
    }

    if (i >= objs_->size()) {
        return nullptr;
    }

    return (*objs_)[i]->get_str();
}

struct status_machine {
    /* 状态码 */
    int status;

    /* 状态机处理函数 */
    const char* (redis_object::*func) (const char*, size_t&);
};

static struct status_machine status_tab[] = {
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
        //printf(">>>parse one: %s<<<\n", data);
        //printf(">>parse one<<<\n");
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

    CHECK(objs_);

    objs_->push_back(obj_);

    if (objs_->size() == (size_t) cnt_) {
        obj_ = nullptr;
        cnt_ = 0;
        status_ = redis_s_finish;
        return data;
    } else {
        obj_ = cache_.get();
        obj_->set_parent(this);
        return parse_object(data, len);
    }
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
    data = get_line(data, len, buf_, found);
    if (!found) {
        assert(len == 0);
        return data;
    }

    if (UNLIKELY(EMPTY(buf_))) {
        status_ = redis_s_null;
        return data;
    }

    type_ = REDIS_OBJ_STATUS;
    status_ = redis_s_finish;
    return data;
}

const char* redis_object::parse_error(const char* data, size_t& len) {
    bool found = false;
    data = get_line(data, len, buf_, found);
    if (!found) {
        assert(len == 0);
        return data;
    }

    if (UNLIKELY(EMPTY(buf_))) {
        status_ = redis_s_null;
        return data;
    }

    type_ = REDIS_OBJ_ERROR;
    status_ = redis_s_finish;
    return data;
}

const char* redis_object::parse_number(const char* data, size_t& len) {
    bool found = false;
    data = get_line(data, len, buf_, found);
    if (!found) {
        assert(len == 0);
        return data;
    }

    if (UNLIKELY(EMPTY(buf_))) {
        status_ = redis_s_null;
        return data;
    }

    type_ = REDIS_OBJ_INTEGER;
    status_ = redis_s_finish;
    return data;
}

const char* redis_object::parse_strlen(const char* data, size_t& len) {
    bool found = false;
    cnt_ = 0;
    data = get_length(data, len, cnt_, found);
    if (status_ == redis_s_null || !found) {
        return data;
    }

    if (cnt_ <= 0) {
        status_ = redis_s_finish;
        return data;
    }

    type_ = REDIS_OBJ_STRING;
    status_ = redis_s_string;

    return parse_string(data, len);
}

const char* redis_object::parse_string(const char* data, size_t& len) {
    buf_.reserve((size_t) cnt_);
    data = get_data(data, len, (size_t) cnt_);
    if (buf_.size() == (size_t) cnt_) {
        status_ = redis_s_strend;
        return parse_strend(data, len);
    }

    return data;
}

const char* redis_object::parse_strend(const char* data, size_t& len) {
    bool found = false;
    std::string buf;
    data = get_line(data, len, buf, found);

    // If the buf_ not empty, some data other '\r\n' got.
    if (UNLIKELY(!EMPTY(buf))) {
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
    bool found = false;
    cnt_ = 0;
    data = get_length(data, len, cnt_, found);
    if (status_ == redis_s_null || !found) {
        return data;
    }

    if (cnt_ <= 0) {
        status_ = redis_s_finish;
        return data;
    }

    type_ = REDIS_OBJ_ARRAY;
    status_ = redis_s_array;

    obj_ = cache_.get();
    obj_->set_parent(this);

    return parse_array(data, len);
}

const char* redis_object::parse_array(const char* data, size_t& len) {
    assert(obj_ != nullptr);

    return parse_object(data, len);
}

const char* redis_object::get_data(const char* data, size_t& len, size_t want) {
    size_t n = buf_.size();
    assert(n < want);

    want -= n;

#if 1
    if (want > len) {
        want = len;
	len = 0;
    } else {
        len -= want;
    }

    buf_.append(data, want);
    data += want;
#else
    for (size_t i = 0; i < want && len > 0; i++) {
        buf_.push_back(*data++);
        len--;
    }
#endif

    return data;
}

const char* redis_object::get_length(const char* data, size_t& len,
      int& res, bool& found) {
    data = get_line(data, len, buf_, found);
    if (!found) {
        assert(len == 0);
        return data;
    }

    if (UNLIKELY(EMPTY(buf_))) {
        status_ = redis_s_null;
        return data;
    }

    // buf_.push_back('\0');  // The c++11 promise the last char is null.
    char* endptr;
    res = (int) strtol(buf_.c_str(), &endptr, 10);
    buf_.clear();

    if (endptr == buf_.c_str() || *endptr != '\0') {
        status_ = redis_s_null;
        return data;
    }
    return data;
}

const char* redis_object::get_line(const char* data, size_t& len,
      std::string& buf, bool& found) {
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
            buf.push_back(*data++);
            len--;
            break;
        }
    }
    return data;
}

bool redis_object::to_string(std::string& out) const {
//#define USE_UNIX_CRLF
#ifdef USE_UNIX_CRLF
#define CRLF    "\n"
#else
#define CRLF    "\r\n"
#endif

#define USE_APPEND

    if (objs_ && !EMPTY(*objs_)) {
#ifdef USE_APPEND
        out.append("*").append(std::to_string(objs_->size())).append(CRLF);
#else
        out += "*";
        out += std::to_string(objs_->size());
        out += CRLF;
#endif

        for (const auto& obj : *objs_) {
            if (!obj->to_string(out)) {
                return false;
            }
        }
    }

    //assert(UNLIKELY(!EMPTY(buf_)));

    switch (type_) {
    case REDIS_OBJ_STATUS:
#ifdef USE_APPEND
        out.append("+").append(buf_.c_str(), buf_.size()).append(CRLF);
#else
	out += "+";
	out.append(buf_.c_str(), buf_.size());
	out += CRLF;
#endif
        break;
    case REDIS_OBJ_ERROR:
#ifdef USE_APPEND
        out.append("-").append(buf_.c_str(), buf_.size()).append(CRLF);
#else
	out += "-";
	out.append(buf_.c_str(), buf_.size());
	out += CRLF;
#endif
        break;
    case REDIS_OBJ_INTEGER:
#ifdef USE_APPEND
        out.append(":").append(buf_.c_str(), buf_.size()).append(CRLF);
#else
	out += ":";
	out.append(buf_.c_str(), buf_.size());
	out += CRLF;
#endif
        break;
    case REDIS_OBJ_STRING:
#ifdef USE_APPEND
        out.append("$").append(std::to_string(buf_.size())).append(CRLF)
            .append(buf_.c_str(), buf_.size()).append(CRLF);
#else
	out += "$";
	out += std::to_string(buf_.size());
	out += CRLF;
	out.append(buf_.c_str(), buf_.size());
	out += CRLF;
#endif
        break;
    //case acl::REDIS_RESULT_ARRAY:
    //    break;
    default:
        break;
    }

    return true;
}

redis_object& redis_object::set_status(const std::string& data,
      bool return_parent) {
    type_ = REDIS_OBJ_STATUS;
    buf_ = data;
    return return_parent ? *parent_ : *this;
}

redis_object& redis_object::set_error(const std::string& data,
      bool return_parent) {
    type_ = REDIS_OBJ_ERROR;
    buf_ = data;
    return return_parent ? *parent_ : *this;
}

redis_object& redis_object::set_number(long long n, bool return_parent) {
    type_ = REDIS_OBJ_INTEGER;
    buf_ = std::to_string(n);
    return return_parent ? *parent_ : *this;
}

redis_object& redis_object::set_string(const std::string &data,
      bool return_parent) {
    type_ = REDIS_OBJ_STRING;
    if (UNLIKELY(!EMPTY(data))) {
        buf_ = data;
    }
    return return_parent ? *parent_ : *this;
}

redis_object& redis_object::create_child() {
    redis_object* obj = cache_.get();
    obj->set_parent(this);

    CHECK(objs_);
    objs_->push_back(obj);

    if (obj_ == nullptr) {
        // The last one is NULL.
        type_ = REDIS_OBJ_ARRAY;
    }

    cnt_ = objs_->size();
    return *obj;
}

} // namespace pkv
