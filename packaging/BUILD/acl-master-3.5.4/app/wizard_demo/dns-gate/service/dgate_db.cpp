#include "stdafx.h"
#include "dgate_db.h"

dgate_db::dgate_db(const char* dbfile, const char* charset /* "utf-8" */) {
	dbfile_ = dbfile;
	db_ = new acl::db_sqlite(dbfile, charset);
}

dgate_db::~dgate_db(void) {
	db_->close();
	delete db_;
}

#define	TABLE_NAME	"name_info"

bool dgate_db::create_table(void) {
	acl::query query;
	query.create("CREATE TABLE IF NOT EXISTS :name_info"
		"(name TEXT NOT NULL,"
		" stamp INTEGER NOT NULL,"
		" stamp_s TEXT NOT NULL,"
		" client TEXT NOT NULL,"
		" PRIMARY KEY(name, stamp)"
		")").set_parameter(TABLE_NAME, TABLE_NAME);
	if (!db_->exec_update(query)) {
		logger_error("create %s table error, sql=%s",
			TABLE_NAME, query.to_string().c_str());
		return false;
	}
	logger("create %s ok", TABLE_NAME);

	query.reset();
	query.create("CREATE INDEX stamp_idx ON :name_info(stamp DESC)")
		.set_parameter(TABLE_NAME, TABLE_NAME);

	if (!db_->exec_update(query)) {
		logger_warn("create index error %s, sql=%s",
			db_->get_error(), query.to_string().c_str());
	}

	return true;
}

bool dgate_db::open(void) {
	if (!db_->open()) {
		logger_error("open db error=%s, dbfile=%s",
			db_->get_error(), dbfile_.c_str());
		return false;
	}

	if (db_->tbl_exists(TABLE_NAME)) {
		return true;
	} else {
		return create_table();
	}
}

bool dgate_db::add(const char* client, const char* name,
		time_t stamp, const char* stamp_s) {
	acl::query query;
	query.create("INSERT INTO name_info(name, stamp, stamp_s, client)"
		" VALUES(:name, :stamp, :stamp_s, :client)")
		.set_parameter("name", name)
		.set_parameter("stamp", (long long) stamp)
		.set_parameter("stamp_s", stamp_s)
		.set_parameter("client", client);

	if (db_->exec_update(query)) {
		return true;
	}

	logger_error("insert error %s, sql=%s",
		db_->get_error(), query.to_string().c_str());
	return false;
}
