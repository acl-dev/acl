#include "stdafx.h"
#include <getopt.h>

class producer : public acl::thread {
public:
	producer(acl::box<int>& box, int max) : box_(box), max_(max) {}
	~producer(void) {}

protected:
	void* run(void)
	{
		for (int i = 0; i < max_; i++) {
			int* n = new int;
			*n = i;
			box_.push(n, true);
		}

		return NULL;
	}

private:
	acl::box<int>& box_;
	int max_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -t type[tbox|tbox_array|mbox] -n count\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int max = 50000000, ch;
	acl::string type("tbox");
	acl::box<int>* box;

	while ((ch = getopt(argc, argv, "ht:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 't':
			type = optarg;
			break;
		case 'n':
			max = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (type == "tbox") {
		box = new acl::tbox<int>;
	} else if (type == "tbox_array") {
		box = new acl::tbox_array<int>;
	} else if (type == "mbox") {
		box = new acl::mbox<int>;
	} else {
		printf("unknown type=%s\r\n", type.c_str());
		return 1;
	}

	producer thr(*box, max);
	thr.start();

	for (int i = 0; i < max; i++) {
		int* n = box->pop();
		assert(*n == i);
		delete n;
	}

	printf("All over, box type=%s, max=%d\r\n", type.c_str(), max);
	thr.wait();

	delete box;
	return 0;
}
