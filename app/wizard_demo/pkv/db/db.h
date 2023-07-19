#pragma once

namespace pkv {

class db;
using shared_db = std::shared_ptr<db>;

class db {
public:
	db(void) {}
	virtual ~db(void) {}

	virtual bool open(const char* path) = 0;
	virtual bool set(const std::string& key, const std::string& value) = 0;
	virtual bool get(const std::string& key, std::string& value) = 0;
	virtual bool del(const std::string& key) = 0;

	static shared_db create_rdb(void);
};

} // namespace pkv
