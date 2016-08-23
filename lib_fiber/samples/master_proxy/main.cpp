#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

static char *var_cfg_remote_addr;

static acl::master_str_tbl var_conf_str_tab[] = {
	{ "remote_addr", "127.0.0.1:8889", &var_cfg_remote_addr },

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

		tcp_proxy(conn, var_cfg_remote_addr);
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

private:
	void tcp_proxy(acl::socket_stream& conn, const char* remote_addr)
	{
		acl::socket_stream peer;

		if (peer.open(remote_addr, 10, 0) == false)
		{
			acl_msg_error("connect %s error %s", remote_addr,
				acl::last_serror());
			return;
		}

		struct pollfd pfd[2];

		memset(&pfd, 0, sizeof(pfd));

		pfd[0].events = POLLIN;
		pfd[0].fd     = conn.sock_handle();

		pfd[1].events = POLLIN;
		pfd[1].fd     = peer.sock_handle();

		while (1)
		{
			int n = poll(pfd, 2, 1000);
			if (n < 0)
			{
				acl_msg_error("poll error %s", acl::last_serror());
				break;
			}

			if (n == 0)
				continue;

			if (pfd[0].revents & POLLIN)
			{
				if (transfer(conn, peer) == false)
					break;
				pfd[0].revents = 0;
			}

			if (pfd[1].revents & POLLIN)
			{
				if (transfer(peer, conn) == false)
					break;
				pfd[1].revents = 0;
			}
		}
	}

	bool transfer(acl::socket_stream& from, acl::socket_stream& to)
	{
		char  buf[8192];

		int ret = from.read(buf, sizeof(buf), false);
		if (ret < 0)
		{
			logger("read over, fd: %d, error: %s",
				from.sock_handle(), acl::last_serror());
			return false;
		}

		if (to.write(buf, ret) < 0)
		{
			logger("write over, fd: %d, error: %s",
				to.sock_handle(), acl::last_serror());
			return false;
		}

		acl::string sbuf(8192);

		while (true)
		{
			if (from.read_peek(sbuf, true) == false)
			{
				if (from.eof())
					return false;
				break;
			}
			if (to.write(sbuf) == -1)
				return false;
		}

		return true;
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
		const char* addr = ":8887";

		printf("listen: %s\r\n", addr);
		mf.run_alone(addr, argc >= 3 ? argv[2] : NULL, 0);
	}
	else
		mf.run_daemon(argc, argv);

	return 0;
}
