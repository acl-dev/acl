#include "stdafx.h"
#include "stamp.h"

class redis_oper
{
public:
	redis_oper(acl::redis_client_cluster& cluster, acl::fiber_sem& sem)
		: cluster_(cluster)
		, sem_(sem)
	{
		nset_ = 0;
		nget_ = 0;
		ndel_ = 0;
	}

	~redis_oper(void)
	{
	}

	int set(int count)
	{
		acl::string key, val;
		acl::redis cmd(&cluster_);

		struct timeval last, now;

		gettimeofday(&last, NULL);

		int i;
		for (i = 0; i < count; i++) {
			key.format("key-%d-%d", acl_fiber_self(), i);
			val.format("val-%d-%d", acl_fiber_self(), i);

			acl::fiber_sem_guard guard(sem_);

			if (cmd.set(key, val) == false) {
				printf("fiber-%d: set error: %s, key: %s\r\n",
					acl_fiber_self(), cmd.result_error(),
					key.c_str());
				break;
			} else if (i < 5)
				printf("fiber-%d: set ok, key: %s\r\n",
					acl_fiber_self(), key.c_str());

			cmd.clear();
			nset_++;
		}

		gettimeofday(&now, NULL);
		double spent = stamp_sub(&now, &last);
		printf("---set spent %.2f ms, count %d, speed: %.2f----\r\n",
			spent, i, (i * 1000) / (spent > 0 ? spent : 1));

		return i;
	}

	int get(int count)
	{
		acl::string key, val;
		acl::redis cmd(&cluster_);

		struct timeval last, now;

		gettimeofday(&last, NULL);

		int i;
		for (i = 0; i < count; i++) {
			key.format("key-%d-%d", acl_fiber_self(), i);

			acl::fiber_sem_guard guard(sem_);

			if (cmd.get(key, val) == false) {
				printf("fiber-%d: get error: %s, key: %s\r\n",
					acl_fiber_self(), cmd.result_error(),
					key.c_str());
				break;
			}

			val.clear();
			cmd.clear();
			nget_++;
		}

		gettimeofday(&now, NULL);
		double spent = stamp_sub(&now, &last);
		printf("---get spent %.2f ms, count %d, speed: %.2f----\r\n",
			spent, i, (i * 1000) / (spent > 0 ? spent : 1));

		return i;
	}

	int del(int count)
	{
		acl::string key, val;
		acl::redis cmd(&cluster_);

		struct timeval last, now;
		gettimeofday(&last, NULL);

		int i;
		for (i = 0; i < count; i++) {
			key.format("key-%d-%d", acl_fiber_self(), i);

			acl::fiber_sem_guard guard(sem_);
			if (cmd.del_one(key) < 0) {
				printf("fiber-%d: del error: %s, key: %s\r\n",
					acl_fiber_self(), cmd.result_error(),
					key.c_str());
				break;
			}

			cmd.clear();
			ndel_++;
		}

		gettimeofday(&now, NULL);
		double spent = stamp_sub(&now, &last);
		printf("---del spent %.2f ms, count %d, speed: %.2f----\r\n",
			spent, i, (i * 1000) / (spent > 0 ? spent : 1));

		return i;
	}

	int nset(void) const
	{
		return nset_;
	}

	int nget(void) const
	{
		return nget_;
	}

	int ndel(void) const
	{
		return ndel_;
	}

private:
	acl::redis_client_cluster& cluster_;
	acl::fiber_sem& sem_;
	int   nset_;
	int   nget_;
	int   ndel_;
};

static int __fibers_count = 2;
static int __fibers_max   = 2;
static int __oper_count = 100;
static struct timeval __tm_begin, __tm_end;

class fiber_redis : public acl::fiber
{
public:
	fiber_redis(acl::redis_client_cluster& cluster, acl::fiber_sem& sem)
		: cluster_(cluster)
		, sem_(sem)
	{
	}

	~fiber_redis()
	{

	}

protected:
	// @override
	void run(void)
	{
		redis_oper oper(cluster_, sem_);

		int n = oper.set(__oper_count);
		(void) oper.get(n);
		(void) oper.del(n);

		static long long __total = 0;
		__total += oper.nset() + oper.nget() + oper.ndel();

		if (--__fibers_count == 0) {
			gettimeofday(&__tm_end, NULL);
			double spent = stamp_sub(&__tm_end, &__tm_begin);
			printf("-------- All fibers over now --------\r\n");
			printf("fibers: %d, count: %lld, spent: %.2f, speed: %.2f\r\n",
				__fibers_max, __total, spent,
				(__total * 1000) / (spent > 0 ? spent : 1));

			//acl::fiber::schedule_stop();
		}
	}

private:
	acl::redis_client_cluster& cluster_;
	acl::fiber_sem& sem_;
};

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s redis_addr\r\n"
		" -n operation_count\r\n"
		" -c fibers count\r\n"
		" -p redis_connections\r\n"
		" -P passwd\r\n"
		" -t conn_timeout\r\n"
		" -r rw_timeout\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i, conn_timeout = 2, rw_timeout = 2, redis_connections = 10;
	acl::string addr("127.0.0.1:6379"), passwd;

	while ((ch = getopt(argc, argv, "hs:n:c:p:r:t:P:")) > 0) {
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
		case 'p':
			redis_connections = atoi(optarg);
			break;
		case 'r':
			rw_timeout = atoi(optarg);
			break;
		case 't':
			conn_timeout = atoi(optarg);
			break;
		case 'P':
			passwd = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl_fiber_msg_stdout_enable(1);

	// declare redis cluster
	acl::redis_client_cluster cluster;
	cluster.set(addr, 0, conn_timeout, rw_timeout);
	cluster.set_password("default", passwd);

	// declare fiber sem
	acl::fiber_sem sem(redis_connections);

	std::vector<fiber_redis*> fibers;

	gettimeofday(&__tm_begin, NULL);

	for (i = 0; i < __fibers_count; i++)
	{
		fiber_redis* fiber = new fiber_redis(cluster, sem);
		fibers.push_back(fiber);
		fiber->start(320000);
	}

	acl_fiber_schedule();

	for (std::vector<fiber_redis*>::iterator it = fibers.begin();
		it != fibers.end(); ++it)
	{
		delete *it;
	}

	return 0;
}
