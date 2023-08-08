#pragma once
#include "db/db.h"
#include "dao_base.h"

namespace pkv {
namespace dao {

class string : public dao_base {
public:
    string();
    ~string() override = default;

    bool set(shared_db& db, const std::string& key, const char* data);
    bool get(shared_db& db, const std::string& key, std::string& out);

public:
    void set_string(const char* data);
    bool to_string(std::string& out);

protected:
    const char* build();

private:
    const char* data_;
};

} // namespace dao
} // namespace pkv
