#include "stdafx.h"
#include "string.h"

namespace pkv {
namespace dao {

string::string() : data_(nullptr) {}

bool string::to_string(std::string& out) {
    auto res = build();
    if (res && *res) {
        out = res;
        return true;
    } else {
        return false;
    }
}

void string::set_string(const char* data) {
    data_ = data;
}

const char* string::build() {
    if (this->finished_) {
        return this->result_;
    }

    this->create_writer();

    yyjson_mut_obj_add_str(this->w_doc_, this->w_root_, "type", "string");
    yyjson_mut_obj_add_int(this->w_doc_, this->w_root_, "expire", -1);
    yyjson_mut_obj_add_str(this->w_doc_, this->w_root_, "data", data_);
    this->result_ = yyjson_mut_write(this->w_doc_, 0, NULL);

    finished_ = true;
    return result_;
}

bool string::set(shared_db& db, const std::string& key, const char* data) {
    data_ = data;
    if (build() == nullptr) {
        return false;
    }
    return this->save(db, key, this->result_);
}

bool string::get(shared_db& db, const std::string& key, std::string& out) {
    auto data = this->read(db, key);
    if (data == nullptr) {
        return false;
    }
    if (strcasecmp(this->type_.c_str(), "string") != 0) {
        logger_error("invalid type=%s, key=%s", type_.c_str(), key.c_str());
        return false;
    }

    auto type = yyjson_get_type(data);
    if (type != YYJSON_TYPE_STR) {
        logger_error("invalid json_node type=%d", (int) type);
        return false;
    }

    auto v = yyjson_get_str(data);
    auto n = yyjson_get_len(data);
    if (v && n > 0) {
        out.append(v, n);
        return true;
    }
    return false;
}

} // namespace dao
} // namespace pkv
