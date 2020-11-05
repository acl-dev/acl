#include "acl_cpp/lib_acl.hpp"
#include <getopt.h>
#include <unistd.h>

const char* CREATE_TBL =
	"create table tbl_test\r\n"
	"(key varchar(128) not null,\r\n"
	" value varchar(128) not null,\r\n"
	" primary key(key))\r\n";

class dbthread : public acl::thread {
public:
	dbthread(const char* dbpath, const char* action, int max, acl::atomic_long& count)
	: dbpath_(dbpath), action_(action), max_(max), count_(count) {}
	~dbthread(void) {}

private:
	acl::string dbpath_;
	acl::string action_;
	int max_;
	acl::atomic_long& count_;

	void* run(void) {
		acl::db_sqlite db(dbpath_, "gbk");

		if (db.open() == false) {
			printf("open %s error\r\n", dbpath_.c_str());
			return NULL;
		}

		if (tbl_create(db, "tbl_test") == false) {
			return NULL;
		}

		int ret;
		if (action_ == "add") {
			ret = dbadd(db, max_);
		} else if (action_ == "get") {
			ret = dbget(db, max_);
		} else if (action_ == "update") {
			ret = dbupdate(db, max_);
		} else if (action_ == "del") {
			ret = dbdel(db, max_);
		} else {
			printf("unknown action: %s\r\n", action_.c_str());
			return NULL;
		}
		printf("db-%s: %d\r\n", action_.c_str(), ret);
		if (ret > 0) {
			count_ += ret;
		}
		return NULL;
	}

	int dbadd(acl::db_handle& db, int max) {
		acl::string sql;
		assert(db.begin_transaction());
		for (int i = 0; i < max; i++) {
			sql.format("insert into tbl_test('key', 'value')"
				" values('key-%d', 'value-%d')", i, i);
			if (db.sql_update(sql) == false) {
				printf("sql_update: |%s| error\r\n", sql.c_str());
				return -1;
			}

			if (i > 0 && i % 1000 == 0) {
				assert(db.commit());
				assert(db.begin_transaction());
			}
		}
		assert(db.commit());
		return max;
	}

	int dbget(acl::db_handle& db, int max) {
		acl::string sql, key, value;
		int i;
		for (i = 0; i < max; i++) {
			key.format("key-%d", i);
			sql.format("select value from tbl_test"
				" where key='%s'", key.c_str());
			if (db.sql_select(sql) == false) {
				printf("sql_select: |%s| error\r\n", sql.c_str());
				return -1;
			}
			if (db.length() == 0) {
				break;
			}
			db.free_result();
		}
		return i;
	}

	int dbupdate(acl::db_handle& db, int max) {
		acl::string sql, key, value;
		for (int i = 0; i < max; i++) {
			key.format("key-%d", i);
			value.format("value-%d-%d", i, i);
			sql.format("update tbl_test set value = '%s'"
				" where key='%s'", value.c_str(), key.c_str());
			if (db.sql_update(sql) == false) {
				printf("sql_update: |%s| error\r\n", sql.c_str());
				return -1;
			}
		}

		return max;
	}

	int dbdel(acl::db_handle& db, int max) {
		acl::string sql, key;
		int n = 0;
		for (int i = 0; i < max; i++) {
			key.format("key-%d", i);
			sql.format("delete from tbl_test where key='%s'",
				key.c_str());
			if (db.sql_update(sql) == false) {
				printf("sql_update: |%s| error\r\n", sql.c_str());
				return -1;
			}
			n += db.affect_count();
		}

		return n;
	}

	bool tbl_create(acl::db_handle& db, const char* tbl_name) {
		if (db.tbl_exists(tbl_name)) {
			return true;
		}
		if (db.sql_update(CREATE_TBL) == false) {
			printf("create table failed\r\n");
			return false;
		}
		return true;
	}
};

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n"
		" -s listen_addr[default: 127.0.0.1:7343\r\n"
		" -n max[default: 1000]\r\n"
		" -c nthreads[default: 1]\r\n"
		" -p dbpath[default: .]\r\n"
		" -l libpath[default: libsqlite3.so]\r\n"
		" -a action[default: get, update|add|get|del\r\n"
		, procname);
}

int main(int argc, char* argv[]) {
	int nthreads = 1, max = 1000, ch;
	acl::string dbpath = "./var", libpath = "./libsqlite3.so";
	acl::string action = "get";
	acl::string addr = "127.0.0.1:7343";

	while ((ch = getopt(argc, argv, "hn:c:p:l:s:a:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max = atoi(optarg);
			break;
		case 'c':
			nthreads = atoi(optarg);
			break;
		case 'p':
			dbpath = optarg;
			break;
		case 'l':
			libpath = optarg;
			break;
		case 's':
			addr = optarg;
			break;
		case 'a':
			action = optarg;
			break;
		default:
			break;
		}
	}

	if (access(libpath.c_str(), R_OK) == -1) {
		printf("access %s failed %s\r\n", libpath.c_str(), acl::last_serror());
		return 1;
	}

	acl::log::stdout_open(true);
	acl::db_handle::set_loadpath(libpath);

	acl::atomic_long success;

	std::vector<acl::thread*> threads;
	for (int i = 0; i < nthreads; i++) {
		acl::string path;
		path << dbpath << "/" << "db" << i << ".db";
		acl::thread* thr = new dbthread(path, action, max, success);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {

		(*it)->wait();
		delete *it;
	}

	printf("All over, nthreads=%d, total=%d, success=%lld\r\n",
		nthreads, nthreads * max, success.value());
	return 0;
}
