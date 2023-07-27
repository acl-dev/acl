//
// Created by shuxin ¡¡¡¡zheng on 2023/7/27.
//

#include "stdafx.h"

#ifdef HAS_WT

#include <wiredtiger.h>
#include "wt_sess.h"
#include "wdb.h"

namespace pkv {

wdb::wdb(size_t cache_max) : db_(nullptr), cache_max_(cache_max) {}

wdb::~wdb() {
    if (db_) {
        db_->close(db_, nullptr);
        logger("wdb in %s closed", path_.c_str());
    }
}

bool wdb::open(const char *path) {
    path_ = path;
    path_ += "/wdb";
    if (acl_make_dirs(path_.c_str(), 0755) == -1) {
        logger_error("create dir=%s error=%s", path_.c_str(), acl::last_serror());
        return false;
    }

    int ret = wiredtiger_open(path_.c_str(), nullptr, "create", &db_);
    if (ret != 0) {
        logger_error("open %s error=%d", path_.c_str(), ret);
        return false;
    }

    return true;
}

bool wdb::set(const std::string &key, const std::string &value) {
    auto sess = get_session();
    if (sess == nullptr) {
        return false;
    }

    bool ret = sess->add(key, value);
    put_session(sess);
    return ret;
}

bool wdb::get(const std::string &key, std::string &value) {
    auto sess = get_session();
    if (sess == nullptr) {
        return false;
    }

    bool ret = sess->get(key, value);
    put_session(sess);
    return ret;
}

bool wdb::del(const std::string &key) {
    auto sess = get_session();
    if (sess == nullptr) {
        return false;
    }

    bool ret = sess->del(key);
    put_session(sess);
    return ret;
}

wt_sess *wdb::get_session() {
    if (sessions_.empty()) {
        auto sess = new wt_sess(*this);
        if (sess->open()) {
            return sess;
        }

        delete sess;
        return nullptr;
    }

    auto sess = sessions_.back();
    sessions_.pop_back();
    return sess;
}

void wdb::put_session(wt_sess* sess) {
    if (sessions_.size() < cache_max_) {
        sessions_.emplace_back(sess);
    } else {
        delete sess;
    }
}

} // namespace pkv

#endif // HAS_WT
