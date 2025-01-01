#include "stdafx.h"
#include <cstdio>
#include <cstdlib>
#include "fiber_pool.h"

class myfibers : public fiber_pool<int> {
public:
    myfibers(int buf, int concurrency, int qlen, int milliseconds, bool thr)
    : fiber_pool<int>(buf, concurrency, qlen, milliseconds, thr)
    {
    }

    ~myfibers() override = default;

    long long get_result() const {
        return result_;
    }

protected:
    long long result_ = 0;

    void run(std::vector<int>& args) override {
        for (auto i : args) {
            if (i < 100) {
                printf("Fiber-%d: got %d\r\n", acl::fiber::self(), i);
            }
        }
        result_ += args.size();
    }
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
    printf("usage: %s -h [help] -n count -b buf -c concurrency -q qlen -t timeout -T [if using in threads]\r\n", procname);
}

int main(int argc, char *argv[]) {
    int  ch;
    bool thr = false;
    int  buf = 500, concurrency = 10, qlen = 0, milliseconds = -1;
    long long count = 100;

    acl::acl_cpp_init();
    acl::log::stdout_open(true);

    while ((ch = getopt(argc, argv, "hn:b:c:q:t:T")) > 0) {
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
            default:
                break;
        }
    }


    std::shared_ptr<myfibers> fbs = std::make_shared<myfibers>
        (buf, concurrency, qlen, milliseconds, thr);

    go[fbs, count] {
        for (long long i = 0; i < count; i++) {
            fbs->add(i);
        }

        fbs->stop();
    };

    struct timeval begin;
    gettimeofday(&begin, nullptr);

    acl::fiber::schedule();

    struct timeval end;
    gettimeofday(&end, nullptr);

    long long result = fbs->get_result();
    double tc = acl::stamp_sub(end, begin);
    double speed = (result * 1000) / tc;
    printf("total result: %lld, tc: %.2f ms, speed: %.2f qps\r\n",
            result, tc, speed);
    return 0;
}
