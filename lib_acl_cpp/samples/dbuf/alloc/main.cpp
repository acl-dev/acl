#include "stdafx.h"
#include <list>
#include <vector>

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/time.h>
#endif

class myobj : public acl::dbuf_obj {
public:
	myobj(acl::dbuf_guard* dbuf, int i) : dbuf_(dbuf), i_(i) {}

	void run(void) {
		for (int i = 0; i < 100; i++) {
			char* s = dbuf_->dbuf_strdup("hello");
			bufs_.push_back(s);
		}
		printf("hello world(%d)\r\n", i_);
	}

	int get(void) const {
		return i_;
	}

	~myobj(void) {}

private:
	acl::dbuf_guard* dbuf_;
	std::vector<char*> bufs_;
	int i_;
};

static void test1(int nblock, int capacity, int incr, int max, int pos) {
	acl::dbuf_guard dbuf((size_t) nblock, (size_t) capacity);
	dbuf.set_increment((size_t) incr);
	myobj* o;
	std::list<myobj*, acl::dbuf_allocator<myobj*> > objs;

	for (int i = 0; i < max; i++) {
		o = dbuf.create<myobj>(&dbuf, i);
		if (i < 10) {
			o->run();
		}
		objs.push_back(o);
	}

	o = (myobj*) dbuf.get((size_t) pos);
	if (o == NULL) {
		printf("get: %d NULL\r\n", pos);
	} else if (o->get() != pos) {
		printf("get: %d != %d\r\n", o->get(), pos);
	} else {
		printf("ok, max: %d, pos: %d\r\n", max, pos);
	}

	printf("\r\n----------------walk the list------------\r\n");
	int n = 0;
	for (std::list<myobj*, acl::dbuf_allocator<myobj*> >::iterator
		it = objs.begin(); it != objs.end(); ++it) {
		(*it)->run();
		if (++n >= 10) {
			break;
		}
	}

	objs.clear();
	dbuf.dbuf_reset();

	printf("\r\n----------------Again---------------\r\n");

	for (int i = 0; i < max; i++) {
		o = dbuf.create<myobj>(&dbuf, i);
		if (i < 10) {
			o->run();
		}
	}

	o = (myobj*) dbuf.get((size_t) pos);

	if (o == NULL) {
		printf("get: %d NULL\r\n", pos);
	} else if (o->get() != pos) {
		printf("get: %d != %d\r\n", o->get(), pos);
	} else {
		printf("ok, max: %d, pos: %d\r\n", max, pos);
	}
}

static void test2(void) {
	acl::dbuf_guard dbuf;
	std::vector<myobj, acl::dbuf_allocator<myobj> > objs;
	int i = 0;

	//objs.reserve(10);

	myobj o1(&dbuf, i++);
	objs.push_back(o1);

	myobj o2(&dbuf, i++);
	objs.push_back(o2);

	myobj o3(&dbuf, i++);
	objs.push_back(o3);

	myobj o4(&dbuf, i++);
	objs.push_back(o4);

	myobj o5(&dbuf, i++);
	objs.push_back(o5);

	printf("objs count is %ld\r\n", (long) objs.size());

	for (std::vector<myobj, acl::dbuf_allocator<myobj> >::iterator
		it = objs.begin(); it != objs.end(); ++it) {
		(*it).run();
	}
}

static void usage(const char* procname) {
	printf("usage: %s -h [help] -n max -p pos -b nblock -c capacity"
		" -i incr\r\n", procname);
}

int main(int argc, char* argv[]) {
	acl::log::stdout_open(true);

	int nblock = 2, capacity = 1000, incr = 999;
	int  n, max = 100000, pos = 987;
	while ((n = getopt(argc, argv, "hn:p:b:c:i:")) > 0) {
		switch (n) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max = atoi(optarg);
			break;
		case 'p':
			pos = atoi(optarg);
			break;
		case 'c':
			capacity = atoi(optarg);
			break;
		case 'i':
			incr = atoi(optarg);
			break;
		case 'b':
			nblock = atoi(optarg);
			break;
		default:
			break;
		}
	}
	
	test1(nblock, capacity, incr, max, pos);

	printf("\r\n");
	printf("=======================================================\r\n");
	test2();

	return 0;
}
