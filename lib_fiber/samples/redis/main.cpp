#include "stdafx.h"
#include "stamp.h"

static int __fibers_count = 2;
static int __fibers_max   = 2;
static int __oper_count = 100;
static struct timeval __begin;
static struct timeval __finish;

//#define USE_PIPELINE

static void fiber_redis(ACL_FIBER *fiber, void *ctx)
{
#ifdef USE_PIPELINE
	acl::redis_client_pipeline* pipeline = (acl::redis_client_pipeline*) ctx;
	acl::redis cmd;
	cmd.set_pipeline(pipeline);
#else
	acl::redis_client_cluster *cluster = (acl::redis_client_cluster *) ctx;
	acl::redis cmd(cluster);
#endif

	acl::string key, val;

	int i = 0;

	struct timeval last, now;

	gettimeofday(&last, NULL);

	for (; i < __oper_count; i++) {
		key.format("key-%d-%d", acl_fiber_id(fiber), i);
		val.format("val-%d-%d", acl_fiber_id(fiber), i);
		if (cmd.set(key, val) == false) {
			printf("fiber-%d: set error: %s, key: %s\r\n",
				acl_fiber_id(fiber), cmd.result_error(), key.c_str());
			break;
		} else if (i < 5)
			printf("fiber-%d: set ok, key: %s\r\n",
				acl_fiber_id(fiber), key.c_str());
		cmd.clear();
	}

	gettimeofday(&now, NULL);
	double spent = stamp_sub(&now, &last);

	printf("---set spent %.2f ms, count %d, speed: %.2f----\r\n",
		spent, i, (i * 1000) / (spent > 0 ? spent : 1));

	gettimeofday(&last, NULL);

	for (int j = 0; j < i; j++) {
		key.format("key-%d-%d", acl_fiber_id(fiber), j);
		if (cmd.get(key, val) == false) {
			printf("fiber-%d: get error: %s, key: %s\r\n",
				acl_fiber_id(fiber), cmd.result_error(), key.c_str());
			break;
		}
		val.clear();
		cmd.clear();
	}

	gettimeofday(&now, NULL);
	spent = stamp_sub(&now, &last);
	printf("---get spent %.2f ms, count %d, speed: %.2f----\r\n",
		spent, i, (i * 1000) / (spent > 0 ? spent : 1));

	gettimeofday(&last, NULL);

	for (int j = 0; j < i; j++) {
		key.format("key-%d-%d", acl_fiber_id(fiber), j);
		if (cmd.del_one(key) < 0) {
			printf("fiber-%d: del error: %s, key: %s\r\n",
				acl_fiber_id(fiber), cmd.result_error(), key.c_str());
			break;
		}
		cmd.clear();
	}

	gettimeofday(&now, NULL);
	spent = stamp_sub(&now, &last);
	printf("---del spent %.2f ms, count %d, speed: %.2f----\r\n",
		spent, i, (i * 1000) / (spent > 0 ? spent : 1));

	if (--__fibers_count == 0) {
		long long total = __fibers_max * i * 3;

		gettimeofday(&__finish, NULL);
		spent = stamp_sub(&__finish, &__begin);
		printf("fibers: %d, count: %lld, spent: %.2f, speed: %.2f\r\n",
			__fibers_max, total, spent,
			(total * 1000) / (spent > 0 ? spent : 1));
		//acl_fiber_schedule_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s redis_addr\r\n"
		" -p passwd\r\n"
		" -n operation_count\r\n"
		" -c fibers count\r\n"
		" -t conn_timeout\r\n"
		" -r rw_timeout\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i, conn_timeout = 2, rw_timeout = 2;
	acl::string addr("127.0.0.1:6379"), passwd;

	while ((ch = getopt(argc, argv, "hs:n:c:r:t:p:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			__oper_count = atoi(optarg);
			break;
		case 'c':
			__fibers_count = atoi(optarg);
			__fibers_max = __fibers_count;
			break;
		case 'r':
			rw_timeout = atoi(optarg);
			break;
		case 't':
			conn_timeout = atoi(optarg);
			break;
		case 'p':
			passwd = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	acl_fiber_msg_stdout_enable(1);

#ifdef USE_PIPELINE
	acl::redis_client_pipeline pipeline(addr.c_str(), conn_timeout,
		rw_timeout);
	pipeline.start();
#else
	acl::redis_client_cluster cluster;
	cluster.set_password("default", passwd);
	cluster.set(addr.c_str(), 0, conn_timeout, rw_timeout);
#endif

	gettimeofday(&__begin, NULL);

	for (i = 0; i < __fibers_count; i++) {
#ifdef USE_PIPELINE
		acl_fiber_create(fiber_redis, &pipeline, 327680);
#else
		acl_fiber_create(fiber_redis, &cluster, 327680);
#endif
	}

	acl_fiber_schedule();

	return 0;
}
