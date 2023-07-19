#include "stdafx.h"
#include "rocksdb/rdb.h"
#include "db.h"

namespace pkv {

class dummy_db : public db {
public:
    dummy_db() = default;
    ~dummy_db() override = default;

    bool open(const char*) override {
        return false;
    }

    bool set(const std::string&, const std::string&) override {
        return false;
    }

    bool get(const std::string&, std::string&) override {
        return false;
    }

    bool del(const std::string&) override {
        return false;
    }
};

shared_db db::create_rdb() {
#ifdef HAS_ROCKSDB
    return std::make_shared<rdb>();
#else
    return std::make_shared<dummy_db>();
#endif
}

} // namespace pkv
