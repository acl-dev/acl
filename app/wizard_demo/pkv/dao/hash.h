#pragma once
#include "db/db.h"
#include "dao_base.h"

namespace pkv {
namespace dao {

class hash : public dao_base {
public:
    hash();
    ~hash() override = default;

    int hdel(shared_db& db, const std::string& key, const std::string& name);

    bool hset(shared_db& db, const std::string& key, const std::string& name,
            const std::string& value);

    bool hget(shared_db& db, const std::string& key, const std::string& name,
            std::string& value);

    bool hgetall(shared_db& db, const std::string& key);

    const std::map<std::string, std::string>& get_fields() {
        return fields_;
    }

public:
    bool to_string(std::string& out);

private:
    std::map<std::string, std::string> fields_;

    const char* build();
};

} // namespace dao
} // namespace pkv
