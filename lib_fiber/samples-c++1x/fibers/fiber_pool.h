#pragma once

template <typename T>
class fiber_pool {
public:
    fiber_pool(int buf, int concurrency, int qlen, int milliseconds, bool thr)
    : qlen_(qlen)
    , milliseconds_(milliseconds)
    {
        for (int i = 0; i < concurrency; i++) {
            wg_.add(1);

            auto fb = go[this, buf, thr] {
                std::shared_ptr<acl::box2<T>> box;

                if (thr) {
                    box = std::make_shared<acl::fiber_tbox2<T>>();
                } else {
                    box = std::make_shared<acl::fiber_sbox2<T>>(buf);
                }

		boxes_.push_back(box);
		fiber_run(box);
            };

            fibers_.push_back(fb);
        }
    }

    virtual ~fiber_pool() = default;

    void add(T t) {
        boxes_[next_++ % boxes_.size()]->push(t, true);
    }

    void stop() {
        for (auto fb : fibers_) {
            acl_fiber_kill(fb);
        }
        wg_.wait();
    }

    virtual void run(std::vector<T>& tasks) = 0;

private:
    using box_ptr = std::shared_ptr<acl::box2<T>>;
    acl::wait_group wg_;
    int qlen_;
    int milliseconds_;
    size_t next_ = 0;
    std::vector<box_ptr> boxes_;
    std::vector<ACL_FIBER*> fibers_;

    void fiber_run(std::shared_ptr<acl::box2<T>> box) {
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
            } else if (acl::fiber::self_killed()) {
                break;
            } else {
		    printf("Wait timeout: %s\r\n", acl::last_serror());
	    }
        }

	printf("fiber-%d: exiting: %s\r\n", acl::fiber::self(), acl::last_serror());
        wg_.done();
    }
};
