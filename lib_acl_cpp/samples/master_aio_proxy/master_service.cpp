#include "stdafx.h"
#include "req_callback.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
// ÅäÖÃÄÚÈÝÏî

static char *var_cfg_backend_addr;
static char *var_cfg_request_file;
static char *var_cfg_respond_file;

acl::master_str_tbl var_conf_str_tab[] = {
	{ "backend_addr", "127.0.0.1:8888", &var_cfg_backend_addr },
	//{ "backend_addr", "webmail.mail.163.com:80", &var_cfg_backend_addr },
	//{ "backend_addr", "sz.mail.store.qq.com:80", &var_cfg_backend_addr },
	{ "request_file", "", &var_cfg_request_file },
	{ "respond_file", "", &var_cfg_respond_file },

	{ 0, 0, 0 }
};

acl::master_bool_tbl var_conf_bool_tab[] = {

	{ 0, 0, 0 }
};

acl::master_int_tbl var_conf_int_tab[] = {

	{ 0, 0 , 0 , 0, 0 }
};

static acl::ofstream* __req_fp = NULL;
static acl::ofstream* __res_fp = NULL;

//////////////////////////////////////////////////////////////////////////////

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
	if (var_cfg_request_file && *var_cfg_request_file)
	{
		__req_fp = new acl::ofstream;
		if (__req_fp->open_trunc(var_cfg_request_file) == false)
		{
			logger_error("open file %s error %s",
				var_cfg_request_file, acl::last_serror());
			delete __req_fp;
			__req_fp = NULL;
		}
	}

	if (var_cfg_respond_file && *var_cfg_respond_file)
	{
		__res_fp = new acl::ofstream;
		if (__res_fp->open_trunc(var_cfg_respond_file) == false)
		{
			logger_fatal("open file %s error %s",
				var_cfg_respond_file, acl::last_serror());
			delete __res_fp;
			__res_fp = NULL;
		}
	}
}

void master_service::proc_on_exit()
{
	if (__req_fp)
		delete __req_fp;
	if (__res_fp)
		delete __res_fp;
}
