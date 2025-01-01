#include "stdafx.h"
#include <cstdio>
#include <cstdlib>

template <typename T>
class fibers {
public:
    fibers(int buf, int concurrency, int qlen, int milliseconds)
    : buf_(buf)
    , concurrency_(concurrency)
    , qlen_(qlen)
    , milliseconds_(milliseconds)
    {
    }

    virtual ~fibers() {}

    void start() {
        for (int i = 0; i < concurrency_; i++) {
            auto fb = go[this] {
                auto box = std::make_shared<acl::fiber_sbox2<T>>(buf_);
                boxes_.push_back(box);
                wait(box);
            };
            fibers_.push_back(fb);
        }
    }

    void add(T t) {
        boxes_[next_++ % boxes_.size()]->push(t, true);
    }

    void stop() {
        for (auto fb : fibers_) {
            acl_fiber_kill(fb);
        }
    }

    virtual void run(std::vector<T>& tasks) = 0;

private:
    using box_ptr = std::shared_ptr<acl::fiber_sbox2<T>>;
    int buf_;
    int concurrency_;
    int qlen_;
    int milliseconds_;
    size_t next_ = 0;
    std::vector<box_ptr> boxes_;
    std::vector<ACL_FIBER*> fibers_;

    void wait(box_ptr box) {
        std::vector<T> tasks;

        while (true) {
            T t;
            if (box->pop(t, milliseconds_)) {
                tasks.emplace_back(t);
                if (tasks.size() >= (size_t) qlen_) {
                    this->run(tasks);
                    tasks.clear();
                }
            } else if (!tasks.empty()) {
                this->run(tasks);
                tasks.clear();
            } else if (acl_fiber_killed(acl_fiber_running())) {
                break;
            }
        }
    }
};

class myfibers : public fibers<int> {
public:
    myfibers(int buf, int concurrency, int qlen, int milliseconds)
    : fibers<int>(buf, concurrency, qlen, milliseconds)
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
    printf("usage: %s -h [help] -n count -b buf -c concurrency -q qlen -t timeout\r\n", procname);
}

int main(int argc, char *argv[]) {
    int  ch;
    int  buf = 500, concurrency = 10, qlen = 0, milliseconds = -1;
    long long count = 100;

    acl::acl_cpp_init();
    acl::log::stdout_open(true);

    while ((ch = getopt(argc, argv, "hn:b:c:q:t:")) > 0) {
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
            default:
                break;
        }
    }


    std::shared_ptr<myfibers> fbs = std::make_shared<myfibers>
        (buf, concurrency, qlen, milliseconds);
    fbs->start();

    go[fbs, count] {
        for (long long i = 0; i < count; i++) {
            fbs->add(i);
        }

        ::sleep(1);
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
