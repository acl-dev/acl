#include "stdafx.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/time.h>
#endif

class myobj : public acl::dbuf_obj
{
public:
	myobj(int i) : i_(i) {}

	void run()
	{
		printf("hello world\r\n");
	}

	int get(void) const
	{
		return i_;
	}

private:
	~myobj() {}
	int i_;
};

static void test(int nblock, int capacity, int incr, int max, int pos)
{
	acl::dbuf_guard dbuf((size_t) nblock, (size_t) capacity);
	dbuf.set_increment((size_t) incr);
	myobj* o;

	for (int i = 0; i < max; i++)
	{
		o = dbuf.create<myobj>(i);
		if (i < 10)
			o->run();
	}

	o = (myobj*) dbuf.get((size_t) pos);
	if (o == NULL)
		printf("get: %d NULL\r\n", pos);
	else if (o->get() != pos)
		printf("get: %d != %d\r\n", o->get(), pos);
	else
		printf("ok, max: %d, pos: %d\r\n", max, pos);

	printf("----------------Again---------------\r\n");
	dbuf.dbuf_reset();

	for (int i = 0; i < max; i++)
	{
		o = dbuf.create<myobj>(i);
		if (i < 10)
			o->run();
	}

	o = (myobj*) dbuf.get((size_t) pos);
	if (o == NULL)
		printf("get: %d NULL\r\n", pos);
	else if (o->get() != pos)
		printf("get: %d != %d\r\n", o->get(), pos);
	else
		printf("ok, max: %d, pos: %d\r\n", max, pos);

}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -n max -p pos -b nblock -c capacity"
		" -i incr\r\n", procname);
}

int main(int argc, char* argv[])
{
	acl::log::stdout_open(true);

	int nblock = 2, capacity = 1000, incr = 999;
	int  n, max = 100000, pos = 987;
	while ((n = getopt(argc, argv, "hn:p:b:c:i:")) > 0)
	{
		switch (n)
		{
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
	
	test(nblock, capacity, incr, max, pos);
	return 0;
}
