#ifndef FIBER_SERVER_H
#define FIBER_SERVER_H

class MainWindow;

class fiber_server : public acl::fiber {
public:
    fiber_server(const char * ip, int port, MainWindow *parent);
    ~fiber_server();

    void stop();

protected:
    void run() override;

private:
    std::string ip_;
    int port_;
    MainWindow *parent_;
    acl::fiber_tbox<bool> *box_;

    void server_start();
};

#endif // FIBER_SERVER_H
