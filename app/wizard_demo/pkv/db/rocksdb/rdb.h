#pragma once

#include "db/db.h"

#ifdef HAS_ROCKSDB

namespace rocksdb {
	class DB;
}

namespace pkv {

class rdb : public db {
public:
	rdb(void);
	~rdb(void);

protected:
	// @override
	bool open(const char* path);

	// @override
	bool set(const std::string& key, const std::string& value);

	// @override
	bool get(const std::string& key, std::string& value);

	// @override
	bool del(const std::string& key);

private:
	std::string path_;
	rocksdb::DB* db_;
};

} // namespace pkv

#endif // HAS_ROCKSDB
