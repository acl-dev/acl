#include "stdafx.h"
#include "redis/redis_object.h"
#include "redis/redis_client.h"
#include "redis/redis_transfer.h"
#include "master_service.h"

char *var_cfg_redis_addrs;
char *var_cfg_redis_pass;

acl::master_str_tbl var_conf_str_tab[] = {
	{ "redis_addrs",	"127.0.0.1:6379",	&var_cfg_redis_addrs	},
	{ "redis_pass",		"",			&var_cfg_redis_pass	},

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

master_service::master_service(void)
: redis_pipeline_(NULL)
{
}

master_service::~master_service(void)
{
}

void master_service::on_accept(acl::socket_stream& conn)
{
	conn.set_rw_timeout(var_cfg_io_timeout);

	acl::dbuf_guard* dbuf = new acl::dbuf_guard;
	redis_client client(conn);

	redis_transfer transfer(*dbuf, conn, *redis_pipeline_);

	std::vector<const redis_object*> objs;

	while (true) {
		if (!client.read_request(dbuf->get_dbuf(), objs)) {
			//logger_error("read client request error!");
			break;
		}

		if (!transfer.run(objs)) {
			break;
		}

		objs.clear();
		dbuf->dbuf_reset(0);
	}

	delete dbuf;
}

void master_service::proc_pre_jail(void)
{
	logger(">>>proc_pre_jail<<<");
}

void master_service::proc_on_listen(acl::server_socket& ss)
{
	logger(">>>listen %s ok<<<", ss.get_addr());
}

void master_service::proc_on_init(void)
{
	//redis_pipeline_ = new acl::redis_client_pipeline(var_cfg_redis_addrs);
	redis_pipeline_ = new acl::fiber_redis_pipeline(var_cfg_redis_addrs);
	if (var_cfg_redis_pass && *var_cfg_redis_pass) {
		redis_pipeline_->set_password(var_cfg_redis_pass);
	}
	redis_pipeline_->start_thread();
}

void master_service::proc_on_exit(void)
{
	if (redis_pipeline_) {
		redis_pipeline_->stop_thread();
		delete redis_pipeline_;
	}
}

bool master_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
