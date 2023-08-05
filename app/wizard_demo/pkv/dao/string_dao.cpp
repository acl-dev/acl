#include "stdafx.h"
#include "string_dao.h"

namespace pkv {

string_dao::string_dao() : data_(nullptr) {}

string_dao::~string_dao() {}

void string_dao::set_string(const char* data) {
    data_ = data;
}

bool string_dao::to_string(std::string& out) {
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val* root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_str(doc, root, "type", "string");
    yyjson_mut_obj_add_int(doc, root, "expire", -1);
    yyjson_mut_obj_add_str(doc, root, "data", data_);
    const char* data = yyjson_mut_write(doc, 0, NULL);

    if (data == nullptr) {
        yyjson_mut_doc_free(doc);
        return false;
    }

    out.append(data);
    yyjson_mut_doc_free(doc);
    return true;
}

} // namespace pkv
