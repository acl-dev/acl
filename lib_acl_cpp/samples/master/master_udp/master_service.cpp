#include "stdafx.h"
#include "master_service.h"

////////////////////////////////////////////////////////////////////////////////
// ÅäÖÃÄÚÈÝÏî

char *var_cfg_str;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "str", "test_msg", &var_cfg_str },

	{ 0, 0, 0 }
};

int  var_cfg_bool;
acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "bool", 1, &var_cfg_bool },

	{ 0, 0, 0 }
};

int  var_cfg_int;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "int", 120, &var_cfg_int, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

long long int  var_cfg_int64;
acl::master_int64_tbl var_conf_int64_tab[] = {
	{ "int64", 120, &var_cfg_int64, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

////////////////////////////////////////////////////////////////////////////////

master_service::master_service()
{
}

master_service::~master_service()
{
}

void master_service::on_read(acl::socket_stream* stream)
{
	char  buf[1024];
	int   ret;

	if ((ret = stream->read(buf, sizeof(buf), false)) == -1)
		logger_error("read from %s error %s",
			stream->get_peer(true), acl::last_serror());
	else if (stream->write(buf, ret) == -1)
		logger_error("write to %s error %s",
			stream->get_peer(true), acl::last_serror());
	if (0)
		logger(">>Peer: %s, Local: %s", stream->get_peer(true),
			stream->get_local(true));
}

void master_service::proc_on_init()
{
	const std::vector<acl::socket_stream*>& sstreams = get_sstreams();
	std::vector<acl::socket_stream*>::const_iterator cit = sstreams.begin();
	for (; cit != sstreams.end(); ++cit)
		logger("local addr: %s, fd: %d", (*cit)->get_local(true),
			(*cit)->sock_handle());
}

void master_service::proc_on_exit()
{
}
