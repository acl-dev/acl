#include "stdafx.h"
#include "key.h"

namespace pkv {
namespace dao {

key::key() {}

bool key::to_string(std::string& out) {
    return true;
}

bool key::del(shared_db& db, const std::string& keyname) {
    return db->del(keyname);
}

bool key::type(shared_db& db, const std::string& keyname, std::string& out) {
    auto data = this->read(db, keyname);
    if (data == nullptr) {
        return false;
    }
    out = this->type_;
    return true;
}

} // namespace dao
} // namespace pkv
