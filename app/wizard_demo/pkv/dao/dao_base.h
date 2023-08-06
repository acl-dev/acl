#pragma once
#include <yyjson.h>
#include "db/db.h"

namespace pkv {
namespace dao {

class dao_base {
public:
    dao_base();
    virtual ~dao_base() = default;

public:
    bool save(shared_db& db, const std::string& key);
    bool read(shared_db& db, const std::string& key, std::string& out);

protected:
    std::string type_;

    virtual const char* build() = 0;
};

} // namespace dao
} // namespace pkv
