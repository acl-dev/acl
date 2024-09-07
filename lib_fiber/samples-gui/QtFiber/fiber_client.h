#ifndef FIBER_CLIENT_H
#define FIBER_CLIENT_H

class fiber_client : public acl::fiber {
public:
    fiber_client(const char *ip, int port, size_t max = 100, int delay = 0);

protected:
    // @override
    void run() override;

private:
    std::string ip_;
    int port_;
    size_t max_;
    int delay_;

    ~fiber_client() = default;
};

#endif // FIBER_CLIENT_H
