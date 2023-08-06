#include "stdafx.h"
#include "string.h"

namespace pkv {
namespace dao {

string::string() : finished_(false), data_(nullptr), result_(nullptr) {
    doc_ = yyjson_mut_doc_new(NULL);
    root_ = yyjson_mut_obj(doc_);
    yyjson_mut_doc_set_root(doc_, root_);
}

string::~string() {
    if (result_) {
        free(result_);
    }
    yyjson_mut_doc_free(doc_);
}

void string::reset() {
    yyjson_mut_doc_free(doc_);

    finished_ = false;
    data_ = nullptr;
    result_ = nullptr;

    doc_ = yyjson_mut_doc_new(NULL);
    root_ = yyjson_mut_obj(doc_);
    yyjson_mut_doc_set_root(doc_, root_);
}

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
    if (finished_) {
        return result_;
    }

    yyjson_mut_obj_add_str(doc_, root_, "type", "string");
    yyjson_mut_obj_add_int(doc_, root_, "expire", -1);
    yyjson_mut_obj_add_str(doc_, root_, "data", data_);
    result_ = yyjson_mut_write(doc_, 0, NULL);

    finished_ = true;
    return result_;
}

bool string::set(shared_db& db, const std::string& key, const char* data) {
    data_ = data;
    return this->save(db, key);
}

bool string::get(shared_db& db, const std::string& key, std::string& out) {
    if (!this->read(db, key, out)) {
        return false;
    }

    if (strcasecmp(type_.c_str(), "string") != 0) {
        logger_error("invalid type=%s, key=%s", type_.c_str(), key.c_str());
        return false;
    }
    return true;
}

} // namespace dao
} // namespace pkv
