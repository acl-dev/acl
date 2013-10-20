#include "stdafx.h"
#include "req_callback.h"
#include "master_service.h"

////////////////////////////////////////////////////////////////////////////////
// ÅäÖÃÄÚÈÝÏî

static char *var_cfg_backend_addr;
static char *var_cfg_request_file;
static char *var_cfg_respond_file;

acl::master_str_tbl var_conf_str_tab[] = {
	{ "backend_addr", "192.168.198.41:8888", &var_cfg_backend_addr },
	//{ "backend_addr", "webmail.mail.163.com:80", &var_cfg_backend_addr },
	//{ "backend_addr", "sz.mail.store.qq.com:80", &var_cfg_backend_addr },
	{ "request_file", "./request.txt", &var_cfg_request_file },
	{ "respond_file", "./request.txt", &var_cfg_respond_file },

	{ 0, 0, 0 }
};

static int   var_cfg_read_line;

acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "read_line", 1, &var_cfg_read_line },

	{ 0, 0, 0 }
};

static int   var_cfg_io_idle_limit;

acl::master_int_tbl var_conf_int_tab[] = {
	{ "io_idle_limit", 60, &var_cfg_io_idle_limit, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

static acl::ofstream __req_fp;
static acl::ofstream __res_fp;

////////////////////////////////////////////////////////////////////////////////

master_service::master_service()
{
}

master_service::~master_service()
{
}

bool master_service::on_accept(acl::aio_socket_stream* client)
{
	logger("connect from %s, fd %d", client->get_peer(true),
		client->sock_handle());

	req_callback* callback = new req_callback(client,
		__req_fp, __res_fp);
	callback->start(var_cfg_backend_addr);
	return true;
}

void master_service::proc_on_init()
{
	if (__req_fp.open_trunc(var_cfg_request_file) == false)
		logger_fatal("open file %s error %s",
			var_cfg_request_file, acl::last_serror());

	if (__res_fp.open_trunc(var_cfg_respond_file) == false)
		logger_fatal("open file %s error %s",
			var_cfg_respond_file, acl::last_serror());
}

void master_service::proc_on_exit()
{
	__req_fp.close();
	__res_fp.close();
}
