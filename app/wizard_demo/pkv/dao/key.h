#pragma once
#include "db/db.h"
#include "dao_base.h"

namespace pkv {
namespace dao {

class key : public dao_base {
public:
    key();
    ~key() override = default;

    bool del(shared_db& db, const std::string& key);
    bool type(shared_db& db, const std::string& key, std::string& out);

public:
    bool to_string(std::string& out);

private:
};

} // namespace dao
} // namespace pkv
