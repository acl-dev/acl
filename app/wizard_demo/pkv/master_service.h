#pragma once

#include "db/db.h"

class master_service : public acl::master_fiber {
public:
    master_service() = default;
    ~master_service() override = default;

    void close_db();

protected:
    // @override
    void on_accept(acl::socket_stream& conn) override;

    // @override
    void proc_on_listen(acl::server_socket& ss) override;

    // @override
    void proc_pre_jail() override;

    // @override
    void proc_on_init() override;

    // @override
    void proc_on_exit() override;

    // @override
    bool proc_on_sighup(acl::string&) override;

private:
    pkv::shared_db db_ = nullptr;

    void run(acl::socket_stream& conn, size_t size);
};
