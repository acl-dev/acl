#pragma once

#include "../db.h"

#ifdef HAS_ROCKSDB

namespace rocksdb {
    class DB;
}

namespace pkv {

class rdb : public db {
public:
    rdb();
    ~rdb() override;

protected:
    // @override
    bool open(const char* path) override;

    // @override
    bool set(const std::string& key, const std::string& value) override;

    // @override
    bool get(const std::string& key, std::string& value) override;

    // @override
    bool del(const std::string& key) override;

public:
    // @override
    const char* get_dbtype() const override {
        return "rdb";
    }

private:
    std::string path_;
    rocksdb::DB* db_;
};

} // namespace pkv

#endif // HAS_ROCKSDB