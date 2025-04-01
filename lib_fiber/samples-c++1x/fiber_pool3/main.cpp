#include "stdafx.h"
#include <getopt.h>
#include <cstdio>
#include <cstdlib>

static void mytest(int i, int doze) {
	printf("Task %d is running in fiber-%d\r\n", i, acl::fiber::self());
	if (doze > 0) {
		acl::fiber::delay(doze);
	}
}

static void run(acl::fiber_pool& pool, int count, int delay, int doze) {
	int i = 0;

	// 在协程池中执行第一个任务，捕获外部变量.
	pool.exec([i, doze]() {
		printf("Task %d is running in fiber-%d\r\n",
			i, acl::fiber::self());
		if (doze > 0) {
			acl::fiber::delay(doze);
		}
	});
	i++;

	if (delay > 0) {
		acl::fiber::delay(delay);
	}

	for (int j = 0; j < count; j++) {
		// 在协程池中执行第二个任务，通过变参方式传递参数.
		pool.exec([doze](int n) {
			printf("Task %d is running in fiber-%d\r\n",
				n, acl::fiber::self());
			if (doze > 0) {
				acl::fiber::delay(doze);
			}
		}, i);
		i++;

		if (delay > 0) {
			acl::fiber::delay(delay);
		}
	}

	// 在协程池中执行第三个任务，将变参传递给普通函数.
	pool.exec(mytest, i, doze);
}

static void usage(const char *procname) {
	printf("usage: %s -h[help]\r\n"
		" -f minimal_fibers[default: 0]\r\n"
		" -t maximal_fibers[default: 10]\r\n"
		" -i idle_timeout[default: 1000 ms]\r\n"
		" -n tasks count[default: 18]\r\n"
		" -C [if check the fiber pool, default: false]\r\n"
		" -A [if create one fiber to add tasks, default: false]\r\n"
		" -d delay_before_add_next_task[default: 0]\r\n"
		" -s sleep_ms_after_exec_task_in_fiber[default: 0]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int ch, min = 0, max = 10, idle = 1000, count = 18, delay = 0, doze = 0;
	bool check = false, alone = false;

	while ((ch = getopt(argc, argv, "hf:t:i:n:CAd:s:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			min = atoi(optarg);
			break;
		case 't':
			max = atoi(optarg);
			break;
		case 'i':
			idle = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'C':
			check = true;
			break;
		case 'A':
			alone = true;
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 's':
			doze = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	// 创建一个协程对象，各个参数如下：
	// min: 最小协程数
	// max: 最大协程数
	// idle: 协程空闲退出时间
	// 协程池中任务队列的缓冲区大小: 500
	// 每个协程栈大小：64000 bytes
	// 协程池中的协程是否采用共享栈模式：否
	acl::fiber_pool pool(min, max, idle, 500, 64000, false);

	if (alone) {
		go[&pool, count, delay, doze] {
			run(pool, count, delay, doze);
		};
	} else {
		delay = 0;
		run(pool, count, delay, doze);
	}

	if (check) {
		go[&pool] {
			while (true) {
				size_t n = pool.get_box_count();
				printf("fibers count is %zd, idle %zd\r\n",
					n, pool.get_box_idle());
				if (n == 0) {
					break;
				}
				acl::fiber::delay(100);
			}
		};
	}

	// 开始协程调度过程.
	acl::fiber::schedule();

	printf("All over now!\r\n");
	return 0;
}
