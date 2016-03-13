#include "stdafx.h"
#include "util.h"

static char  __local_addr[64];
static char  __server_addr[64];
static bool  __server_fixed = true;

static void udp_read_callback(int, ACL_EVENT*, ACL_VSTREAM* in,
	void *context)
{
	char buf[4096];
	int  ret;
	acl::socket_stream* out = (acl::socket_stream*) context;

	ret = acl_vstream_read(in, buf, sizeof(buf));
	if (ret == ACL_VSTREAM_EOF)
	{
		printf("read error: %s\r\n", acl::last_serror());
		return;
	}

	if (out->write(buf, ret) == -1)
		printf("write error: %s\r\n", acl::last_serror());
}

static void stdin_read_callback(int, ACL_EVENT*, ACL_VSTREAM* in,
	void *context)
{
	char buf[4096];
	int  ret;
	acl::socket_stream* out = (acl::socket_stream*) context;

	ret = acl_vstream_read(in, buf, sizeof(buf));
	if (ret == ACL_VSTREAM_EOF)
	{
		printf("read error: %s\r\n", acl::last_serror());
		return;
	}

	if (out->write(buf, ret) == -1)
		printf("write error: %s\r\n", acl::last_serror());
}

static void run(ACL_EVENT* event)
{
	acl::socket_stream udp;

	// 绑定本地地址
	if (udp.bind_udp(__local_addr) == false)
	{
		printf("bind addr %s error %s\r\n",
			__server_addr, acl::last_serror());
		return;
	}

	// 设置远程服务地址
	else
		udp.set_peer(__server_addr);

	udp.set_rw_timeout(100);
	ACL_VSTREAM* udp_stream = udp.get_vstream();

	acl::socket_stream stdio_in;
	stdio_in.open(0);

	ACL_VSTREAM* in = stdio_in.get_vstream();
	acl::socket_stream out;
	out.open(2);

	udp.set_tcp_non_blocking(true);
	stdio_in.set_tcp_non_blocking(true);

	acl_event_enable_read(event, in, 0, stdin_read_callback, &udp);
	acl_event_enable_read(event, udp_stream, 0, udp_read_callback, &out);

	out.write("Escape character is '^]'.\r\n");

	while (true)
	{
		acl_event_loop(event);
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-s server_addr [default: 127.0.0.1:8088]\r\n"
		"	-l local_addr [default: 127.0.0.1:18088]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int   ch;

	acl::acl_cpp_init();

	snprintf(__server_addr, sizeof(__server_addr), "127.0.0.1:8088");
	snprintf(__local_addr, sizeof(__local_addr), "0.0.0.0:18088");

	while ((ch = getopt(argc, argv, "hs:l:o")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(__server_addr, sizeof(__server_addr), "%s", optarg);
			break;
		case 'l':
			snprintf(__local_addr, sizeof(__local_addr), "%s", optarg);
			break;
		case 'o':
			__server_fixed = false;
			break;
		default:
			break;
		}
	}

	ACL_EVENT* event = acl_event_new_select(1, 0);
	run(event);

	return 0;
}
