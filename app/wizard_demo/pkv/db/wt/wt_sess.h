//
// Created by shuxin ¡¡¡¡zheng on 2023/7/27.
//

#pragma once

#ifdef HAS_WT

#include <wiredtiger.h>

namespace pkv {

class wdb;

class wt_sess {
public:
    explicit wt_sess(wdb& db);
    ~wt_sess();

    bool open();

    bool add(const std::string& key, const std::string& value);

    bool get(const std::string& key, std::string& value);

    bool del(const std::string& key);

private:
    wdb& db_;

    WT_SESSION *sess_;
    WT_CURSOR  *curs_;
};

} // namespace pkv

#endif // HAS_WT
