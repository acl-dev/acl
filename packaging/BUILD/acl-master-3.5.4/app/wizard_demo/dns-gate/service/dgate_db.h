#pragma once

class dgate_db {
public:
	dgate_db(const char* dbfile, const char* charset = "utf-8");
	~dgate_db(void);

	bool open(void);
	bool add(const char* client, const char* name,
		time_t stamp, const char* stamp_s);

private:
	acl::string dbfile_;
	acl::db_sqlite* db_;

	bool create_table(void);
};
