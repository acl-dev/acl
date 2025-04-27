#include "stdafx.h"
#include <cxxabi.h>
#include <sys/time.h>

class myobj {
public:
	myobj(bool use_typeid) : use_typeid_(use_typeid) {
		if (use_typeid) {
			ACL_OBJ_INC();
		} else {
			ACL_COUNTER_INC(myobj);
		}
	}

	virtual ~myobj() {
		if (use_typeid_) {
			ACL_OBJ_DEC();
		} else {
			ACL_COUNTER_DEC(myobj);
		}
	}

protected:
	bool use_typeid_;
};

class myobj1 : public myobj {
public:
	myobj1(bool use_typeid) : myobj(use_typeid) {
		if (use_typeid) {
			ACL_OBJ_INC();
		} else {
			ACL_COUNTER_INC(myobj1);
		}
	}

	~myobj1() {
		if (use_typeid_) {
			ACL_OBJ_DEC();
		} else {
			ACL_COUNTER_DEC(myobj1);
		}
	}
};

class myobj2 : public myobj {
public:
	myobj2(bool use_typeid) : myobj(use_typeid) {
		if (use_typeid) {
			ACL_OBJ_INC();
		} else {
			ACL_COUNTER_INC(myobj2);
		}
	}

	~myobj2() {
		if (use_typeid_) {
			ACL_OBJ_DEC();
		} else {
			ACL_COUNTER_DEC(myobj2);
		}
	}
};

static void test(size_t max, bool use_typeid) {
	std::vector<myobj*> objs;

	struct timeval begin, end;
	gettimeofday(&begin, NULL);

	for (size_t i = 0; i < max; i++) {
		myobj* o = new myobj1(use_typeid);
		objs.push_back(o);
	}

	for (size_t i = 0; i < max; i++) {
		myobj* o = new myobj2(use_typeid);
		objs.push_back(o);
	}

	gettimeofday(&end, NULL);
	double tc = acl::stamp_sub(end, begin);

	printf("After add, time cost=%.2f ms, obj: %lld, obj1: %lld, obj2: %lld\r\n",
		tc, ACL_COUNTER_COUNT(myobj), ACL_COUNTER_COUNT(myobj1),
		ACL_COUNTER_COUNT(myobj2));

	ACL_COUNTER_PRINT();

	printf("Enter any key to continue...\r\n");
	getchar();

	for (std::vector<myobj*>::iterator it = objs.begin();
		it != objs.end(); ++it) {
		delete *it;
	}

	printf("After del objs: %lld\r\n", ACL_COUNTER_COUNT(myobj));
	ACL_COUNTER_PRINT();
}

int main(int argc, char *argv[]) {
	// ³õÊ¼»¯ acl ¿â
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	size_t max = 1000000;
	if (argc > 1) {
		max = atol(argv[1]);
	}

	if (max == 0 || max > 10000000) {
		max = 1000000;
	}

	printf("use_typeid: yes, use_lock: yes\r\n");
	test(max, true);
	printf("----------------------------------------------------\r\n\r\n");

	printf("use_typeid: no, use_lock: yes\r\n");
	test(max, false);
	printf("----------------------------------------------------\r\n\r\n");

	ACL_COUNTER_INIT(false);

	printf("use_typeid: yes, use_lock: yes\r\n");
	test(max, true);
	printf("----------------------------------------------------\r\n\r\n");

	printf("use_typeid: no, use_lock: no\r\n");
	test(max, false);

	return 0;
}
