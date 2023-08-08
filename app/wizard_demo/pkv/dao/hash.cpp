#include "stdafx.h"
#include "dao/hash.h"

namespace pkv {
namespace dao {

hash::hash() {}

bool hash::to_string(std::string& out) {
    return true;
}

const char* hash::build() {
    if (finished_) {
        return result_;
    }

    this->create_writer();

    yyjson_mut_obj_add_str(this->w_doc_, this->w_root_, "type", "hash");
    yyjson_mut_obj_add_int(this->w_doc_, this->w_root_, "expire", -1);

    auto data = yyjson_mut_obj(this->w_doc_);
    yyjson_mut_obj_add_val(this->w_doc_, this->w_root_, "data", data);

    for (const auto& cit : fields_) {
        yyjson_mut_obj_add_str(this->w_doc_, data, cit.first.c_str(),
                cit.second.c_str());
    }

    this->result_ = yyjson_mut_write(this->w_doc_, 0, NULL);

    finished_ = true;
    return result_;
}

bool hash::hset(shared_db& db, const std::string& key, const std::string& name,
        const std::string& value) {

    (void) hgetall(db, key);
    fields_[name] = value;

    if (build() == nullptr) {
        return false;
    }
    //printf(">>>%s<<<\r\n", this->result_);
    return this->save(db, key, this->result_);
}

// { "type": "hash", "expire": -1, "data": { "name1": "value1", "name2": "value2" }}

bool hash::hget(shared_db& db, const std::string& key, const std::string& name,
        std::string& value) {
    auto data = this->read(db, key);
    if (data == nullptr) {
        return false;
    }
    if (strcasecmp(type_.c_str(), "hash") != 0) {
        return false;
    }

    yyjson_obj_iter iter;
    yyjson_obj_iter_init(data, &iter);

    yyjson_val* vkey, *vval;
    while ((vkey = yyjson_obj_iter_next(&iter))) {
        vval = yyjson_obj_iter_get_val(vkey);
        if (yyjson_equals_str(vkey, name.c_str())) {
            auto v = yyjson_get_str(vval);
            if (v) {
                value = v;
            }
            return true;
        }
    }

    return false;
}

bool hash::hgetall(shared_db& db, const std::string& key) {
    fields_.clear();

    auto data = this->read(db, key);
    if (data == nullptr) {
        return false;
    }
    if (strcasecmp(type_.c_str(), "hash") != 0) {
        return false;
    }

    yyjson_obj_iter iter;
    yyjson_obj_iter_init(data, &iter);

    yyjson_val* vkey, *vval;
    while ((vkey = yyjson_obj_iter_next(&iter))) {
        vval = yyjson_obj_iter_get_val(vkey);
        auto n = yyjson_get_str(vkey);
        auto v = yyjson_get_str(vval);
        if (n && v) {
            fields_[n] = v;
        }
    }

    return true;
}

} // namespace dao
} // namespace pkv
