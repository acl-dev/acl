#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/thread_mutex.hpp"
#endif

#include "acl_cpp/stdlib/class_counter.hpp"

namespace acl {

class_counter::class_counter(bool clean)
: clean_(clean)
{
	lock_ = NEW thread_mutex;
}

class_counter::~class_counter() {
	delete lock_;
}

void class_counter::init(bool thread_safe) {
	if (thread_safe) {
		if (lock_ == NULL) {
			lock_ = NEW thread_mutex;
		}
	} else if (lock_) {
		delete lock_;
		lock_ = NULL;
	}
}

void class_counter::inc(const char *name) {
	if (lock_) {
		lock_->lock();
	}

	std::map<std::string, long long>::iterator it = names_.find(name);
	if (it == names_.end()) {
		names_[name] = 1;
	} else {
		it->second++;
	}

	if (lock_) {
		lock_->unlock();
	}
}

void class_counter::dec(const char *name) {
	if (lock_) {
		lock_->lock();
	}

	std::map<std::string, long long>::iterator it = names_.find(name);
	if (it != names_.end()) {
		if (--it->second == 0 && clean_) {
			names_.erase(it);
		}
	} else {
		logger_error("not find flag %s", name);
	}

	if (lock_) {
		lock_->unlock();
	}
}

long long class_counter::count(const char *name) {
	if (lock_) {
		lock_->lock();
	}

	long long ret;

	std::map<std::string, long long>::const_iterator it = names_.find(name);
	if (it == names_.end()) {
		ret = 0;
	} else {
		ret = it->second;
	}

	if (lock_) {
		lock_->unlock();
	}
	return ret;
}

void class_counter::print(const char *flag) {
	string buf(2048);
	string tmp;

	if (lock_) {
		lock_->lock();
	}

	std::map<std::string, long long>::const_iterator it = names_.begin();
	for (; it != names_.end(); ++it) {
		if (it != names_.begin()) {
			buf.append(", ");
		}
		tmp.format("[%s: %lld]", it->first.c_str(), it->second);
		buf.append(tmp);
	}

	if (lock_) {
		lock_->unlock();
	}

	if (flag) {
		logger("class count(%s)=%s", flag, buf.c_str());
	} else {
		logger("class count=%s", buf.c_str());
	}
}

}  // namespace acl
