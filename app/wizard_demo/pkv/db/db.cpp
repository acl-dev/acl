#include "stdafx.h"
#include "rocksdb/rdb.h"
#include "db.h"

namespace pkv {

class dummy_db : public db {
public:
	dummy_db(void) {}
	~dummy_db(void) {}

	bool open(const char*) {
		return false;
	}

	bool set(const std::string&, const std::string&) {
		return false;
	}

	bool get(const std::string&, std::string&) {
		return false;
	}

	bool del(const std::string&) {
		return false;
	}
};

shared_db db::create_rdb(void) {
#ifdef HAS_ROCKSDB
	return std::make_shared<rdb>();
#else
	return std::make_shared<dummy_db>();
#endif
}

} // namespace pkv
