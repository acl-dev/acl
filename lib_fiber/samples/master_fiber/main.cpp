#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "http_servlet.h"

static char *var_cfg_debug_msg;

static acl::master_str_tbl var_conf_str_tab[] = {
	{ "debug_msg", "test_msg", &var_cfg_debug_msg },

	{ 0, 0, 0 }
};

static int  var_cfg_debug_enable;

static acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "debug_enable", 1, &var_cfg_debug_enable },

	{ 0, 0, 0 }
};

static int  var_cfg_io_timeout;

static acl::master_int_tbl var_conf_int_tab[] = {
	{ "io_timeout", 120, &var_cfg_io_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

//////////////////////////////////////////////////////////////////////////

class master_fiber_test : public acl::master_fiber
{
public:
	master_fiber_test(void) {}
	~master_fiber_test(void) {}

protected:
	// @override
	void on_accept(acl::socket_stream& conn)
	{
		acl_msg_info(">>>accept connection: %d", conn.sock_handle());
		conn.set_rw_timeout(0);
		acl::memcache_session session("127.0.0.1:11211");
		http_servlet servlet(&conn, &session);
		servlet.setLocalCharset("gb2312");

		while (true)
		{
			if (servlet.doRun() == false)
				break;
		}

		acl_msg_info(">>>close one connection: %d, %s",
			conn.sock_handle(), acl::last_serror());
	}

	// @override
	void proc_pre_jail(void)
	{
		acl_msg_info(">>>proc_pre_jail<<<");
	}

	// @override
	void proc_on_init(void)
	{
		acl_msg_info(">>>proc_on_init<<<");
	}

	// @override
	void proc_on_exit(void)
	{
		acl_msg_info(">>>proc_on_exit<<<");
	}
};

int main(int argc, char *argv[])
{
	master_fiber_test& mf =
		acl::singleton2<master_fiber_test>::get_instance();

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	// 设置配置参数表
	mf.set_cfg_int(var_conf_int_tab);
	mf.set_cfg_int64(NULL);
	mf.set_cfg_str(var_conf_str_tab);
	mf.set_cfg_bool(var_conf_bool_tab);

	if (argc >= 2 && strcasecmp(argv[1], "alone") == 0)
	{
		const char* addr = ":8888";

		printf("listen: %s\r\n", addr);
		mf.run_alone(addr, argc >= 3 ? argv[2] : NULL, 0);
	}
	else
		mf.run_daemon(argc, argv);

	return 0;
}
