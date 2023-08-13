//
// Created by zsx on 2023/8/13.
//

#pragma once
#include "db/db.h"

namespace pkv {

class test_db {
public:
    test_db();
    ~test_db();

    static void bench(const std::string& path, size_t max);
    static void bench(shared_db& db, size_t max);
    static size_t bench_set(shared_db& db, size_t max);
    static size_t bench_get(shared_db& db, size_t max);
    static void rdb_bench(const std::string& path, size_t max);
    static void wdb_bench(const std::string& path, size_t max);
};

} // namespace pkv