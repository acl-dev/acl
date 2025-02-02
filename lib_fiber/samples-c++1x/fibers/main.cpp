#include "stdafx.h"
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include "fiber_pool.h"
#include "fiber_pool2.h"

class myfibers : public fiber_pool<int> {
public:
    myfibers(int buf, int concurrency, int qlen, int milliseconds, bool thr)
    : fiber_pool<int>(buf, concurrency, qlen, milliseconds, thr)
    {
    }

    ~myfibers() override = default;

    size_t get_count() const {
        return count_;
    }

protected:
    long long count_ = 0;

    void run(std::vector<int>& args) override {
        for (auto i : args) {
            if (i < 100) {
                printf("Fiber-%d: got %d\r\n", acl::fiber::self(), i);
            }
        }
        count_ += (long long) args.size();
    }
};

static void test1(long long count, int buf, int concurrency, int qlen,
      int milliseconds, bool thr) {
    std::shared_ptr<myfibers> fbs = std::make_shared<myfibers>
            (buf, concurrency, qlen, milliseconds, thr);

    go[fbs, count] {
        for (long long i = 0; i < count; i++) {
            fbs->add(i);
            if (i % 1000000 == 0) {
                acl::fiber::yield();
            }
        }

	printf("Post over!\r\n");
        ::sleep(1);
        fbs->stop();
    };

    struct timeval begin;
    gettimeofday(&begin, nullptr);

    acl::fiber::schedule();

    struct timeval end;
    gettimeofday(&end, nullptr);

    long long cnt = fbs->get_count();
    double tc = acl::stamp_sub(end, begin);
    double speed = (cnt * 1000) / tc;
    printf("total result: %lld, tc: %.2f ms, speed: %.2f qps\r\n",
        cnt, tc, speed);
}

//////////////////////////////////////////////////////////////////////////////

static void task_run(std::atomic_long &total, long long i) {
    total++;
    if (i <= 100) {
        printf("fiber-%d: got %lld\r\n", acl::fiber::self(), i);
    }
}

static void test2(long long count, int buf, int concurrency,
      int milliseconds, bool thr, bool use_exec2) {
    std::shared_ptr<fiber_pool2> fibers = std::make_shared<fiber_pool2>
            (buf, concurrency, milliseconds, thr);

    std::atomic_long total(0);
    long long delay = thr ? 10000 : 1000000;

    go[fibers, count, delay, &total, use_exec2] {
        for (long long i = 0; i < count; i++) {
            if (use_exec2) {
                fibers->exec2(task_run, std::ref(total), i);
            } else {
                fibers->exec(task_run, std::ref(total), i);
            }

            if (i % delay == 0) {
                acl::fiber::yield();
            }
        }

        ::sleep(1);
        fibers->stop();
    };

    struct timeval begin;
    gettimeofday(&begin, nullptr);

    acl::fiber::schedule();

    struct timeval end;
    gettimeofday(&end, nullptr);

    long long cnt = total.load();
    double tc = acl::stamp_sub(end, begin);
    double speed = (cnt * 1000) / tc;
    printf("%s total result: %lld, tc: %.2f ms, speed: %.2f qps\r\n",
        use_exec2 ? "exec2" : "exec", cnt, tc, speed);
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
    printf("usage: %s -h [help]"
		    " -n count\r\n"
		    " -b buf\r\n"
		    " -c concurrency\r\n"
		    " -q qlen\r\n"
		    " -t timeout\r\n"
                    " -D [if use exec for testing fiber_pool2]\r\n"
		    " -T [if using in threads]\r\n", procname);
}

int main(int argc, char *argv[]) {
    int  ch;
    bool thr = false, use_exec2 = true;
    int  buf = 500, concurrency = 10, qlen = 0, milliseconds = -1;
    long long count = 100;

    acl::acl_cpp_init();
    acl::log::stdout_open(true);

    while ((ch = getopt(argc, argv, "hn:b:c:q:t:TD")) > 0) {
        switch (ch) {
            case 'h':
                usage(argv[0]);
                return 0;
            case 'n':
                count = atoll(optarg);
                break;
            case 'b':
                buf = atoi(optarg);
                break;
            case 'c':
                concurrency = atoi(optarg);
                break;
            case 'q':
                qlen = atoi(optarg);
                break;
            case 't':
                milliseconds = atoi(optarg);
                break;
            case 'T':
                thr = true;
                break;
            case 'D':
                use_exec2 = false;
                break;
            default:
                break;
        }
    }

    test1(count, buf, concurrency, qlen, milliseconds, thr);

    printf("-----------------------------------------------------------\r\n");

    test2(count, buf, concurrency, milliseconds, thr, use_exec2);
    return 0;
}
