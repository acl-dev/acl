#pragma once
#include <yyjson.h>
#include "db/db.h"
#include "dao_base.h"

namespace pkv {
namespace dao {

class string : public dao_base {
public:
    string();
    ~string() override;

    bool set(shared_db& db, const std::string& key, const char* data);
    bool get(shared_db& db, const std::string& key, std::string& out);

public:
    void reset();
    void set_string(const char* data);
    bool to_string(std::string& out);

protected:
    const char* build() override;

private:
    bool finished_;
    const char* data_;
    char* result_;
    yyjson_mut_doc* doc_;
    yyjson_mut_val* root_;
};

} // namespace dao
} // namespace pkv
