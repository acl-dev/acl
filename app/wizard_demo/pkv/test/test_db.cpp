//
// Created by zsx on 2023/8/13.
//

#include "stdafx.h"

#include "test_db.h"

namespace pkv {

test_db::test_db() {}

test_db::~test_db() {}

void test_db::bench(const std::string &path, size_t max) {
    rdb_bench(path, max);
    wdb_bench(path, max);
}

void test_db::rdb_bench(const std::string &path, size_t max) {
    auto db = db::create_rdb();
    if (!db->open(path.c_str())) {
        printf("%s: open %s db error %s\r\n",
               __func__, path.c_str(), acl::last_serror());
    } else {
        bench(db, max);
        // Close the db
        db = nullptr;
    }
}
void test_db::wdb_bench(const std::string &path, size_t max) {
    auto db = db::create_wdb();
    if (!db->open(path.c_str())) {
        printf("%s: open %s db error %s\r\n",
               __func__, path.c_str(), acl::last_serror());
    } else {
        bench(db, max);
        // Close the db
        db = nullptr;
    }
}

void test_db::bench(shared_db &db, size_t max) {
    printf("Begin test %s, count=%zd\r\n", db->get_dbtype(), max);

    struct timeval begin;
    gettimeofday(&begin, nullptr);

    size_t n = bench_set(db, max);

    struct timeval end;
    gettimeofday(&end, nullptr);
    double cost = acl::stamp_sub(end, begin);
    double speed = (n * 1000) / (cost > 0 ? cost : 0.0001);
    printf("%s: count=%zd, cost=%.2f ms, set speed=%.2f\r\n",
           db->get_dbtype(), n, cost, speed);

    gettimeofday(&begin, nullptr);
    n = bench_get(db, max);
    gettimeofday(&end, nullptr);

    cost = acl::stamp_sub(end, begin);
    speed = (n * 1000) / (cost > 0 ? cost : 0.0001);
    printf("%s: count=%zd, cost=%.2f ms, get speed=%.2f\r\n",
           db->get_dbtype(), n, cost, speed);
}

size_t test_db::bench_set(pkv::shared_db &db, size_t max) {
    size_t i;
    for (i = 0; i < max; i++) {
        std::string key("key-");
        key += std::to_string(i);

        std::string val("val-");
        val += std::to_string(i);
        if (!db->set(key, val)) {
            printf("%s: dbtype=%s, set error, key=%s, val=%s\r\n",
                   __func__, db->get_dbtype(), key.c_str(), val.c_str());
            break;
        }
    }
    return i;
}

size_t test_db::bench_get(pkv::shared_db &db, size_t max) {
    size_t i;
    for (i = 0; i < max; i++) {
        std::string key("key-");
        key += std::to_string(i);

        std::string val;
        if (!db->get(key, val)) {
            printf("%s: dbtype=%s, set error, key=%s\r\n",
                   __func__, db->get_dbtype(), key.c_str());
            break;
        }
    }
    return i;
}

} // namespace pkv