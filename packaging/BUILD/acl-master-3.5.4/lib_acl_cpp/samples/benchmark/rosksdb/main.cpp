#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"
#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

using namespace rocksdb;

class db_thread : public acl::thread {
public:
	db_thread(int id, DB* db, const char* action, int max)
	: id_(id), db_(db), action_(action), max_(max) {}
	~db_thread(void) {
		delete db_;
	}

private:
	// @override
	void* run(void) {
		if (action_ == "add") {
			add(max_);
		} else if (action_ == "get") {
			get(max_);
		} else if (action_ == "del") {
			del(max_);
		} else {
			printf("invalid action=%s\r\n", action_.c_str());
		}

		return NULL;
	}

	void add(int max) {
		Status s;
		acl::string key, value;
		int i;
		for (i = 0; i < max; i++) {
			key.format("key-%d-%d", id_, i);
			value.format("value-%d", i);

#if 0
			WriteBatch batch;
			batch.Put(key.c_str(), value.c_str());
			if (i > 0 && i % 1000 == 0) {
				s = db->Write(WriteOptions(), &batch);
			}
#else
			s = db_->Put(WriteOptions(), key.c_str(), value.c_str());
#endif
			if (!s.ok()) {
				printf("Put failed: %s, key=%s, value=%s\r\n",
					s.getState(), key.c_str(), value.c_str());
				break;
			}
		}

		printf("add over, n=%d\r\n", i);
	}

	void get(int max) {
		Status s;
		acl::string key;
		std::string value;
		int i;
		for (i = 0; i < max; i++) {
			key.format("key-%d-%d", id_, i);
			s = db_->Get(ReadOptions(), key.c_str(), &value);
			if (!s.ok()) {
				printf("Get failed, key=%s, error=%s\r\n",
					key.c_str(), s.getState());
				break;
			}
		}

		printf("get over, n=%d\r\n", i);
	}

	void del(int max) {
		Status s;
		acl::string key;
		int i;
		for (i = 0; i < max; i++) {
			key.format("key-%d-%d", id_, i);
			s = db_->Delete(WriteOptions(), key.c_str());
			if (!s.ok()) {
				printf("Del failed, key=%s, error=%s\r\n",
					key.c_str(), s.getState());
				break;
			}
		}

		printf("del over, n=%d\r\n", i);
	}

private:
	int id_;
	DB* db_;
	acl::string action_;
	int max_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -c threads_count\r\n"
		" -a action[default: get, add|get|del]\r\n"
		" -n max_loop\r\n"
		, procname);		
}

int main(int argc, char* argv[]) {
	const char* dbpath = "./db";
	int ch, max = 1000, nthread = 1;
	acl::string action("get");

	while ((ch = getopt(argc, argv, "hn:a:c:")) > 0) {
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
		default:
			break;
		}
	}

	std::vector<acl::thread*> threads;
	for (int i = 0; i < nthread; i++) {
		DB* db;
		Options options;
		options.IncreaseParallelism();
		//	options.OptimizeLevelStyleCompaction();
		options.create_if_missing = true;

		acl::string path;
		path << dbpath << i;
		Status s = DB::Open(options, path.c_str(), &db);
		if (!s.ok()) {
			printf("Open rockdb(%s) failed(%s)!\r\n",
				dbpath, s.getState());
			return 1;
		}

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
