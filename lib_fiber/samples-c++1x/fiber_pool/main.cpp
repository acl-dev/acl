#include "stdafx.h"
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <atomic>

static void task_run(acl::wait_group* wg, std::atomic_long* res, long long i) {
	(*res) += i;
	wg->done();
}

static void test(long long count, size_t min, size_t max, size_t buf,
		int idle_ms, bool shared) {
	std::shared_ptr<acl::fiber_pool> fibers = std::make_shared<acl::fiber_pool>
		(min, max, idle_ms, buf, 64000, shared);

	std::shared_ptr<acl::wait_group> wg(new acl::wait_group);
	std::shared_ptr<std::atomic_long> result(new std::atomic_long(0));

	wg->add(1);
	go[wg, fibers, count, result] {
		printf("Begin to add tasks...\r\n");
		for (long long i = 0; i < count; i++) {
			wg->add(1);
			fibers->exec(task_run, wg.get(), result.get(), i);
		}
		printf("Add task end!\r\n");
		wg->done();
	};

	go[wg, result, fibers, count] {
		struct timeval begin;
		gettimeofday(&begin, nullptr);

		printf("Begin to wait...\r\n");
		wg->wait();
		printf("All task finished!\r\n");

		struct timeval end;
		gettimeofday(&end, nullptr);

		long long res = result->load();
		double tc = acl::stamp_sub(end, begin);
		double speed = (count * 1000) / (tc > 0 ? tc : 0.0000000001);
		printf("Result: %lld, count: %lld, tc: %.2f ms, speed: %.2f qps\r\n",
			res, count, tc, speed);

		fibers->stop();
	};

	acl::fiber::schedule();
}

static void usage(const char* procname) {
	printf("usage: %s -h [help]"
		" -n count\r\n"
		" -b buf\r\n"
		" -t fiber_wait_timeout\r\n"
		" -S [if using shared stack]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
    int ch;
    size_t buf = 500;
    int idle_ms = -1;
    long long count = 100;
    bool shared = false;

    acl::acl_cpp_init();
    acl::log::stdout_open(true);

    while ((ch = getopt(argc, argv, "hn:b:t:S")) > 0) {
        switch (ch) {
            case 'h':
                usage(argv[0]);
                return 0;
            case 'n':
                count = atoll(optarg);
                break;
            case 'b':
                buf = (size_t) atoi(optarg);
                break;
            case 't':
                idle_ms = atoi(optarg);
                break;
	    case 'S':
		shared = true;
		break;
            default:
                break;
        }
    }

    test(count, 10, 100, buf, idle_ms, shared);
    return 0;
}
