#pragma once
#include <yyjson.h>
#include "db/db.h"

namespace pkv {
namespace dao {

class dao_base {
public:
    dao_base();
    virtual ~dao_base();

public:
    bool save(shared_db& db, const std::string& key, const std::string& data);
    yyjson_val* read(shared_db& db, const std::string& key);

protected:
    bool finished_;
    std::string type_;
    char* result_;
    yyjson_mut_doc* w_doc_;
    yyjson_mut_val* w_root_;
    yyjson_doc* r_doc_;
    yyjson_val* r_root_;

    void create_writer();

    yyjson_doc* get_reader_doc() const {
        return r_doc_;
    }

    yyjson_val* get_reader_root() const {
        return r_root_;
    }
};

} // namespace dao
} // namespace pkv
