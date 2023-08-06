#include "stdafx.h"
#include "dao_base.h"

namespace pkv {
namespace dao {

dao_base::dao_base() {}

bool dao_base::read(shared_db& db, const std::string& key, std::string& out) {
    if (!db->get(key, out)) {
        return true;
    }

    yyjson_doc* doc = yyjson_read(out.c_str(), out.size(), 0);
    out.clear();

    if (doc == nullptr) {
        return false;
    }

    yyjson_val* root = yyjson_doc_get_root(doc);
    yyjson_val* val = yyjson_obj_get(root, "type");
    if (val == nullptr) {
        yyjson_doc_free(doc);
        return false;
    }

    auto type = yyjson_get_str(val);
    auto len = yyjson_get_len(val);
    if (type && len > 0) {
        type_ = type;
    } else {
        yyjson_doc_free(doc);
        return false;
    }

    val = yyjson_obj_get(root, "data");
    if (val == nullptr) {
        yyjson_doc_free(doc);
        return false;
    }

    auto data = yyjson_get_str(val);
    len = yyjson_get_len(val);
    if (data && len > 0) {
        out.append(data, len);
        yyjson_doc_free(doc);
        return true;
    }

    yyjson_doc_free(doc);
    return false;
}

bool dao_base::save(shared_db& db, const std::string& key) {
    auto data = this->build();
    if (data && db->set(key, data)) {
        return true;
    }

    return false;
}

} // namespace dao
} // namespace pkv
