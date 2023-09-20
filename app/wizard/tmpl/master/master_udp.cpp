#include "stdafx.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
{
}

master_service::~master_service(void)
{
}

static acl::atomic_long __counter;

void master_service::on_read(acl::socket_stream* stream)
{
	int   n;
	char  buf[512];

	if ((n = stream->read(buf, sizeof(buf), false)) == -1) {
		return;
	}

	long long cnt = ++__counter;

	if (cnt < 100) {
		buf[n] = 0;
		logger("read from %s, %d bytes: %s", stream->get_peer(), n, buf);
	} else if (cnt % 10000 == 0) {
		buf[n] = 0;

		char tmp[1024];
		snprintf(tmp, sizeof(tmp), "count=%lld, %s", cnt, buf);
		acl::meter_time("Read udp", __LINE__, tmp);
	}

	stream->write(buf, n);
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
	logger(">>>proc_on_init<<<");
}

void master_service::proc_on_exit(void)
{
	logger(">>>proc_on_exit<<<");
}

bool master_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
