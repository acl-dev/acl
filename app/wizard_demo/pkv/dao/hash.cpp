#include "stdafx.h"
#include "dao/hash.h"

namespace pkv {
namespace dao {

hash::hash() {}

bool hash::to_string(std::string& out) {
    return true;
}

const char* hash::build() {
    if (this->finished_) {
        return this->result_;
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

    this->finished_ = true;
    return result_;
}

bool hash::hset(shared_db& db, const std::string& key, const std::string& name,
        const std::string& value) {

    (void) hgetall(db, key);
    fields_[name] = value;

    if (build() == nullptr) {
        logger_error("build for hset error, key=%s, name=%s, value=%s",
              key.c_str(), name.c_str(), value.c_str());
        return false;
    }
    //printf(">>>%s<<<\r\n", this->result_);
    return this->save(db, key, this->result_);
}

int hash::hdel(shared_db& db, const std::string& key, const std::string& name) {
    if (!hgetall(db, key)) {
        return -1;
    }
    auto it = fields_.find(name);
    if (it == fields_.end()) {
        return 0;
    }

    fields_.erase(it);

    if (fields_.empty()) {
        db->del(key);
    } else if (build() == nullptr) {
        logger_error("build for hset error, key=%s, name=%s",
              key.c_str(), name.c_str());
        return -1;
    } else if (!this->save(db, key, this->result_)) {
        return -1;
    }

    return 1;
}

// { "type": "hash", "expire": -1, "data": { "name1": "value1", "name2": "value2" }}

bool hash::hget(shared_db& db, const std::string& key, const std::string& name,
        std::string& value) {
    auto data = this->read(db, key);
    if (data == nullptr) {
        logger_error("db read error, key=%s", key.c_str());
        return false;
    }
    if (strcasecmp(type_.c_str(), "hash") != 0) {
        logger_error("nvalid type=%s, key=%s", type_.c_str(), key.c_str());
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
        logger_error("db read error, key=%s", key.c_str());
        return false;
    }
    if (strcasecmp(type_.c_str(), "hash") != 0) {
        logger_error("nvalid type=%s, key=%s", type_.c_str(), key.c_str());
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
