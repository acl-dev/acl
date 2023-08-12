//
// Created by shuxin ¡¡¡¡zheng on 2023/7/27.
//

#include "stdafx.h"

#ifdef HAS_WT

#include <wiredtiger.h>
#include "wt_sess.h"
#include "wdb.h"

namespace pkv {

size_t __cache_max = 10000;
static __thread std::vector<wt_sess*>* __sessions = nullptr;

static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;

static void thread_on_exit(void*) {
        for (std::vector<wt_sess*>::iterator it = __sessions->begin();
                it != __sessions->end(); ++it) {
            delete *it;
        }

        delete __sessions;
}

static acl_pthread_key_t __free_sess_key;

static void thread_init_once() {
    acl_pthread_key_create(&__free_sess_key, thread_on_exit);
}

static wt_sess *get_session(wdb& db) {
    if (__sessions == nullptr) {
        __sessions = new std::vector<wt_sess*>;
        acl_pthread_once(&once_control, thread_init_once);
    } else if (!__sessions->empty()) {
        auto sess = __sessions->back();
        __sessions->pop_back();
        return sess;
    }

    auto sess = new wt_sess(db);
    if (sess->open()) {
        return sess;
    }

    delete sess;
    return nullptr;
}

static void put_session(wt_sess* sess) {
    if (__sessions->size() < __cache_max) {
        __sessions->emplace_back(sess);
    } else {
        delete sess;
    }
}

wdb::wdb(size_t cache_max) : db_(nullptr) {
    __cache_max = cache_max;
}

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
    auto sess = get_session(*this);
    if (sess == nullptr) {
        return false;
    }

    bool ret = sess->add(key, value);
    put_session(sess);
    return ret;
}

bool wdb::get(const std::string &key, std::string &value) {
    auto sess = get_session(*this);
    if (sess == nullptr) {
        return false;
    }

    bool ret = sess->get(key, value);
    put_session(sess);
    return ret;
}

bool wdb::del(const std::string &key) {
    auto sess = get_session(*this);
    if (sess == nullptr) {
        return false;
    }

    bool ret = sess->del(key);
    put_session(sess);
    return ret;
}

} // namespace pkv

#endif // HAS_WT
