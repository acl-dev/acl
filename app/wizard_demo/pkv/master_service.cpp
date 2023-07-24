#include "stdafx.h"
#include "proto/redis_coder.h"
#include "action/redis_handler.h"
#include "master_service.h"

static char *var_cfg_dbpath;

acl::master_str_tbl var_conf_str_tab[] = {
	{ "dbpath",		"./dbpath",	&var_cfg_dbpath		},

	{ 0, 0, 0 }
};

static int  var_cfg_debug_enable;

acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "debug_enable",	1,		&var_cfg_debug_enable	},

	{ 0, 0, 0 }
};

static int  var_cfg_io_timeout;

acl::master_int_tbl var_conf_int_tab[] = {
     { "io_timeout",		120,		&var_cfg_io_timeout, 0, 0 },

     { 0, 0 , 0 , 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {
    { 0, 0 , 0 , 0, 0 }
};


//////////////////////////////////////////////////////////////////////////

using namespace pkv;

void master_service::on_accept(acl::socket_stream& conn) {
    //logger(">>>accept connection: %d", conn.sock_handle());
    //conn.set_rw_timeout(var_cfg_io_timeout);

    pkv::redis_coder parser;
    char buf[20480];

    while(true) {
        int ret = conn.read(buf, sizeof(buf) - 1, false);
        if (ret <= 0) {
            break;
        }

        buf[ret] = 0;

	//if (ret >= 512) { printf("[%s]\r\n", buf); }

        size_t len = (size_t) ret;
        const char* data = parser.update(buf, len);
        auto obj = parser.get_curr();
        assert(obj);
        if (obj->failed()) {
            break;
        }

        //printf("len=%zd, data=%s\r\n", len, data);
        assert(*data == '\0' && len == 0);

        pkv::redis_handler handler(db_, parser, conn);
        if (!handler.handle()) {
            break;
        }

        parser.clear();
    }
}

void master_service::proc_pre_jail() {
    logger(">>>proc_pre_jail<<<");
}

void master_service::proc_on_listen(acl::server_socket& ss) {
    logger(">>>listen %s ok<<<", ss.get_addr());
}

void master_service::proc_on_init() {
    logger(">>>proc_on_init<<<");
    db_ = db::create_rdb();
    if (!db_->open(var_cfg_dbpath)) {
        logger_error("open db(%s) error %s", var_cfg_dbpath, acl::last_serror());
        exit(1);
    }
}

void master_service::proc_on_exit() {
    logger(">>>proc_on_exit<<<");
}

bool master_service::proc_on_sighup(acl::string&) {
    logger(">>>proc_on_sighup<<<");
    return true;
}
