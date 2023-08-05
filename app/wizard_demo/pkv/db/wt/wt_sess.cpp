//
// Created by shuxin ¡¡¡¡zheng on 2023/7/27.
//

#include "stdafx.h"
#include "wdb.h"
#include "wt_sess.h"

#ifdef HAS_WT

namespace pkv {

wt_sess::wt_sess(wdb &db) : db_(db), sess_(nullptr), curs_(nullptr) {}

wt_sess::~wt_sess() = default;

bool wt_sess::open() {
    int ret = db_.get_db()->open_session(db_.get_db(), nullptr, nullptr, &sess_);
    if (ret != 0) {
        logger_error("open session error %d", ret);
        return false;
    }

    ret = sess_->create(sess_, "table:access", "key_format=S, value_format=S");
    if (ret != 0) {
        logger_error("create session error %d", ret);
        return false;
    }

    ret = sess_->open_cursor(sess_, "table:access", nullptr, nullptr, &curs_);
    if (ret != 0) {
        logger_error("open cursor error %d", ret);
        return false;
    }

    return true;
}

bool wt_sess::add(const std::string &key, const std::string &value) {
    assert(curs_);
    curs_->set_key(curs_, key.c_str());
    curs_->set_value(curs_, value.c_str());
    int ret = curs_->insert(curs_);
    if (ret != 0) {
        logger_error("insert %s %s error=%d", key.c_str(), value.c_str(), ret);
        curs_->reset(curs_);
        return false;
    }
    curs_->reset(curs_);
    return true;
}

bool wt_sess::get(const std::string &key, std::string &value) {
    assert(curs_);
    curs_->set_key(curs_, key.c_str());
    int ret = curs_->search(curs_);
    if (ret != 0) {
        logger("search key=%s error=%d", key.c_str(), ret);
        curs_->reset(curs_);
        return false;
    }

    const char* v;
    ret = curs_->get_value(curs_, &v);
    if (ret != 0) {
        logger_error("get_value key=%s errort=%d", key.c_str(), ret);
        curs_->reset(curs_);
        return false;
    }

    value = v;
    curs_->reset(curs_);
    return true;
}

bool wt_sess::del(const std::string &key) {
    assert(curs_);
    curs_->set_key(curs_, key.c_str());
    int ret = curs_->remove(curs_);
    if (ret != 0 && ret != WT_NOTFOUND) {
        logger_error("remove key=%s error=%d", key.c_str(), ret);
        curs_->reset(curs_);
        return false;
    }

    curs_->reset(curs_);
    return true;
}

} // namespace pkv

#endif // HAS_WT
