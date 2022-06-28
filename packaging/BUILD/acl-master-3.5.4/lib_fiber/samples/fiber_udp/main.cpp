#include "stdafx.h"
#include "../stamp.h"

static void fiber_client(acl::socket_stream& stream, int max, bool echo)
{
	acl::string buf;
	acl::string data("hello world\r\n");

	printf("client fiber-%d begin\r\n", acl::fiber::self());

	struct timeval begin;
	gettimeofday(&begin, NULL);

	int i;
	for (i = 0; i < max; i++)
	{
		if (stream.write(data) == -1)
		{
			printf("client fiber-%d write error %s\r\n",
				acl::fiber::self(), acl::last_serror());
			break;
		}

		if (echo && stream.read(buf, false) == false)
		{
			printf("client fiber-%d read error %s\r\n",
				acl::fiber::self(), acl::last_serror());
			break;
		}
	}

	struct timeval end;
	gettimeofday(&end, NULL);
	double spent = stamp_sub(&end, &begin);

	printf("total %d, spent: %.2f, speed: %.2f\r\n",
		i, spent, ((long long) i * 1000) / (spent > 1 ? spent : 1));

	acl::fiber::schedule_stop();
}

static void fiber_server(acl::socket_stream& stream, int max, bool echo)
{
	acl::string buf;

	printf("server fiber-%d begin\r\n", acl::fiber::self());

	struct timeval begin;
	gettimeofday(&begin, NULL);

	int i;
	for (i = 0; i < max; i++)
	{
		if (stream.read(buf, false) == false)
		{
			printf("server fiber-%d read error %s\r\n",
				acl::fiber::self(), acl::last_serror());
			break;
		}
		if (i < 10)
			printf("server fiber-%d read: %s",
				acl::fiber::self(), buf.c_str());

		if (echo && stream.write(buf) == -1)
		{
			printf("server fiber-%d write error %s\r\n",
				acl::fiber::self(), acl::last_serror());
			break;
		}
	}

	struct timeval end;
	gettimeofday(&end, NULL);
	double spent = stamp_sub(&end, &begin);

	printf("total %d, spent: %.2f, speed: %.2f\r\n",
		i, spent, ((long long) i * 1000) / (spent > 1 ? spent : 1));

	acl::fiber::schedule_stop();
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		" -s addr\r\n"
		" -n max_loop\r\n"
		" -c max_fibers\r\n"
		" -E [true if echo client's req]\r\n"
		" -C [true in client_mode]\r\n", procname);
}

int main(int argc, char* argv[])
{
	acl::string addr("127.0.0.1:8188");
	bool client_mode = false, echo = false;
	int  ch, max = 1000, max_fibers = 1;

	while ((ch = getopt(argc, argv, "hs:c:Cn:E")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'c':
			max_fibers = atoi(optarg);
			break;
		case 'C':
			client_mode = true;
			break;
		case 'n':
			max = atoi(optarg);
			break;
		case 'E':
			echo = true;
			break;
		default:
			break;
		}
	}

	acl::socket_stream stream;
	if (client_mode)
	{
		const char* local = "127.0.0.1:0";
		if (stream.bind_udp(local) == false)
		{
			printf("bind_udp error: %s, addr: %s\r\n",
				acl::last_serror(), local);
			return 1;
		}
		stream.set_peer(addr);

		for (int i = 0; i < max_fibers; i++)
		{
			go[&] {
				fiber_client(stream, max, echo);
			};
		}
	}
	else
	{
		if (stream.bind_udp(addr) == false)
		{
			printf("bind_udp error: %s, addr: %s\r\n",
				acl::last_serror(), addr.c_str());
			return 1;
		}

		for (int i = 0; i < max_fibers; i++)
		{
			go[&] {
				fiber_server(stream, max, echo);
			};
		}
	}

	acl::fiber::schedule();
	return 0;
}
