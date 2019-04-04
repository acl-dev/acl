#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"
#include "wiredtiger.h"

static acl::atomic_long __count = 0;
static int __inter = 10000;

class wdb {
public:
	wdb(const char* home)
	: home_(home)
	, conn_(NULL)
	{}

	~wdb(void) {
		if (conn_) {
			conn_->close(conn_, NULL);
		}
	}

	bool open(void) {
		if (conn_) {
			return true;
		}

		int ret = wiredtiger_open(home_.c_str(), NULL, "create", &conn_);
		if (ret != 0) {
			printf("open %s failed, ret=%d\r\n", home_.c_str(), ret);
			return false;
		}
		return true;
	}

	WT_CONNECTION* get_conn(void) const {
		return conn_;
	}

private:
	acl::string    home_;
	WT_CONNECTION *conn_;
};

class wdb_sess {
public:
	wdb_sess(wdb& db)
	: db_(db)
	, session_(NULL)
	, cursor_(NULL) {}

	~wdb_sess(void) {}

	bool open(void) {
		bool ret = db_.get_conn()->open_session(db_.get_conn(),
				NULL, NULL, &session_);
		if (ret != 0) {
			printf("open session failed, ret=%d\r\n", ret);
			return false;
		}

		ret = session_->create(session_, "table:access",
				"key_format=S, value_format=S");
		if (ret != 0) {
			printf("create session failed, ret=%d\r\n", ret);
			return false;
		}

		ret = session_->open_cursor(session_, "table:access",
				NULL, NULL, &cursor_);
		if (ret != 0) {
			printf("create table failed, ret=%d\r\n", ret);
			return false;
		}
		return true;
	}

	bool add(const char* key, const char* value) {
		assert(cursor_);
		cursor_->set_key(cursor_, key);
		cursor_->set_value(cursor_, value);
		int ret = cursor_->insert(cursor_);
		if (ret != 0) {
			printf("insert %s %s failed, ret=%d\r\n", key, value, ret);
			return false;
		}
		cursor_->reset(cursor_);
		return true;
	}

	bool get(const char* key, acl::string& value) {
		assert(cursor_);
		cursor_->set_key(cursor_, key);
		int ret = cursor_->search(cursor_);
		if (ret != 0) {
			printf("search %s failed, ret=%d\r\n", key, ret);
			return false;
		}

		const char* v;
		ret = cursor_->get_value(cursor_, &v);
		if (ret != 0) {
			printf("get_value %s failed, ret=%d\r\n", key, ret);
			return false;
		}

		value = v;
		cursor_->reset(cursor_);
		return true;
	}

	bool del(const char* key) {
		assert(cursor_);
		cursor_->set_key(cursor_, key);
		int ret = cursor_->remove(cursor_);
		if (ret != 0) {
			printf("remove %s failed, ret=%d\r\n", key, ret);
			return false;
		}

		cursor_->reset(cursor_);
		return true;
	}

private:
	wdb&           db_;
	WT_SESSION    *session_;
	WT_CURSOR     *cursor_;
};

class db_thread : public acl::thread {
public:
	db_thread(int id, wdb& db, const char* action, int max)
	: id_(id), db_(db), action_(action), max_(max) {}

	~db_thread(void) {}

private:
	// @override
	void* run(void) {
		wdb_sess sess(db_);
		if (sess.open() == false) {
			printf("open db session failed\r\n");
			return NULL;
		}

		if (action_ == "add") {
			add(sess, max_);
		} else if (action_ == "get") {
			get(sess, max_);
		} else if (action_ == "del") {
			del(sess, max_);
		} else {
			printf("invalid action=%s\r\n", action_.c_str());
		}

		return NULL;
	}

	void add(wdb_sess& sess, int max) {
		acl::string key, value;
		int i;
		for (i = 0; i < max; i++) {
			key.format("key-%d-%d", id_, i);
			value.format("value-%d", i);
			bool ret = sess.add(key.c_str(), value.c_str());
			if (!ret) {
				printf("add failed, key=%s, value=%s\r\n",
					key.c_str(), value.c_str());
				break;
			}
		}

		printf("add over, n=%d\r\n", i);
	}

	void get(wdb_sess& sess, int max) {
		acl::string key, value;
		int i;
		long long n;
		for (i = 0; i < max; i++) {
			key.format("key-%d-%d", id_, i);
			bool ret = sess.get(key.c_str(), value);
			if (!ret) {
				printf("Get failed, key=%s\r\n", key.c_str());
				break;
			} else if (i < 10) {
				printf("key=%s, value=%s\r\n",
					key.c_str(), value.c_str());
			}
			n = ++__count;
			if (n % __inter == 0) {
				char buf[128];
				snprintf(buf, sizeof(buf), "count=%lld, value=%s",
					n, value.c_str());
				acl::meter_time(__FILE__, __LINE__, buf);
			}
		}

		printf("get over, n=%d\r\n", i);
	}

	void del(wdb_sess& sess, int max) {
		acl::string key;
		int i;
		for (i = 0; i < max; i++) {
			key.format("key-%d-%d", id_, i);
			bool ret = sess.del(key.c_str());
			if (!ret) {
				printf("Del failed, key=%s\r\n", key.c_str());
				break;
			}
		}

		printf("del over, n=%d\r\n", i);
	}

private:
	int  id_;
	wdb& db_;
	acl::string action_;
	int  max_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -c threads_count\r\n"
		" -a action[default: get, add|get|del]\r\n"
		" -n max_loop\r\n"
		" -i inter\r\n"
		, procname);		
}

int main(int argc, char* argv[]) {
	const char* dbpath = "./db";
	int ch, max = 1000, nthread = 1;
	acl::string action("get");

	while ((ch = getopt(argc, argv, "hn:a:c:i:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max = atoi(optarg);
			break;
		case 'a':
			action = optarg;
			break;
		case 'c':
			nthread = atoi(optarg);
			break;
		case 'i':
			__inter = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::string path;
	path << dbpath;
	wdb db(path);
	if (!db.open()) {
		printf("open db(%s) error\r\n", dbpath);
		return 1;
	}

	std::vector<acl::thread*> threads;
	for (int i = 0; i < nthread; i++) {
		acl::thread* thr = new db_thread(i, db, action, max);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait();
		delete *it;
	}

	return 0;
}
