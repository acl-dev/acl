#include "stdafx.h"
#include "action/guard_action.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
// ≈‰÷√ƒ⁄»›œÓ

char *var_cfg_redis_addrs;
char *var_cfg_redis_passwd;
char *var_cfg_main_service_list;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "redis_addrs", "127.0.0.1:6379", &var_cfg_redis_addrs },
	{ "redis_passwd", "", &var_cfg_redis_passwd },
	{ "main_service_list", "", &var_cfg_main_service_list },

	{ 0, 0, 0 }
};

acl::master_bool_tbl var_conf_bool_tab[] = {

	{ 0, 0, 0 }
};

int   var_cfg_threads_max;
int   var_cfg_threads_idle;
int   var_cfg_redis_conn_timeout;
int   var_cfg_redis_rw_timeout;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "threads_max", 256, &var_cfg_threads_max, 0, 0 },
	{ "threads_idle", 60, &var_cfg_threads_idle, 0, 0 },
	{ "redis_conn_timeout", 10, &var_cfg_redis_conn_timeout, 0, 0 },
	{ "redis_rw_timeout", 10, &var_cfg_redis_rw_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {

	{ 0, 0 , 0 , 0, 0 }
};

acl::redis_client_cluster var_redis;
std::map<acl::string, bool> var_main_service_list;

//////////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
{
}

master_service::~master_service(void)
{
}

void master_service::on_read(acl::socket_stream* stream)
{
	int   n;
	char  buf[1500];

	if ((n = stream->read(buf, sizeof(buf) - 1, false)) == -1)
		return;

	buf[n] = 0;
	logger("read from %s, %d bytes, buf=|%s|", stream->get_peer(), n, buf);

	const char* peer_ip = stream->get_peer();
	if (peer_ip == NULL || *peer_ip == 0) {
		logger_error("can't get peer ip");
		return;
	}

	guard_action* job = new guard_action(peer_ip, buf);
	threads_->execute(job);
}

void master_service::thread_on_init(void)
{
	logger(">>thread_on_init<<<");
}

void master_service::proc_on_bind(acl::socket_stream&)
{
	logger(">>>proc_on_bind<<<");
}

void master_service::proc_on_init(void)
{
	threads_ = new acl::thread_pool;
	threads_->set_limit(var_cfg_threads_max);
	threads_->set_idle(var_cfg_threads_idle);
	threads_->start();

	var_redis.init(NULL, var_cfg_redis_addrs, var_cfg_threads_max,
		var_cfg_redis_conn_timeout, var_cfg_redis_rw_timeout);
	var_redis.set_password("default", var_cfg_redis_passwd);

	if (var_cfg_main_service_list && *var_cfg_main_service_list)
	{
		ACL_ARGV *tokens  = acl_argv_split(
			var_cfg_main_service_list, ",; \t\r\n");
		ACL_ITER iter;
		acl_foreach(iter, tokens)
		{
			const char* ptr = (const char*) iter.data;
			var_main_service_list[ptr] = true;
		}
		acl_argv_free(tokens);
	}

	logger(">>>proc_on_init<<<");
}

void master_service::proc_on_exit(void)
{
	delete threads_;
	logger(">>>proc_on_exit<<<");
}

bool master_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
