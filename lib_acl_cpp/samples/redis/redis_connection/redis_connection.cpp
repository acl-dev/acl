#include "stdafx.h"

static bool test_auth(acl::redis_connection& redis)
{
	acl::string passwd("hello");

	redis.clear();
	if (redis.auth(passwd.c_str()) == false)
	{
		printf("auth failed, passwd: %s, eof: %s\r\n",
			passwd.c_str(), redis.eof() ? "yes" : "no");
		return false;
	}
	printf("auth ok, passwd: %s\r\n", passwd.c_str());
	return true;
}

static bool test_echo(acl::redis_connection& redis, int n)
{
	acl::string buf("hello world!");

	for (int i = 0; i < n; i++)
	{
		redis.clear();
		if (redis.echo(buf.c_str()) == false)
		{
			printf("echo error: %s\r\n", redis.result_error());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("echo ok\r\n");
	}
	return true;
}

static bool test_ping(acl::redis_connection& redis, int n)
{
	for (int i = 0; i < n; i++)
	{
		redis.clear();
		if (redis.ping() == false)
		{
			printf("ping failed: %s\r\n", redis.result_error());
			return false;
		}
		else if (i < 10)
			printf("ping ok\r\n");
	}
	return true;
}

static bool test_quit(acl::redis_connection& redis)
{
	redis.clear();
	if (redis.quit() == false)
	{
		printf("quit error: %s\r\n", redis.result_error());
		return false;
	}
	else
	{
		printf("quit ok\r\n");
		return true;
	}
}

static bool test_select(acl::redis_connection& redis, int n)
{
	for (int i = 0; i < n; i++)
	{
		redis.clear();
		if (redis.select(i % 2) == false)
		{
			printf("select %d error: %s\r\n",
				i % 2, redis.result_error());
			return false;
		}
		else if (i < 10)
			printf("select %d ok\r\n", i % 2);
	}
	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-n count\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 10]\r\n"
		"-S [if slice request, default: no]\r\n"
		"-a cmd[auth|echo|ping|quit|select]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), cmd;
	bool slice_req = false;

	while ((ch = getopt(argc, argv, "hs:n:C:I:a:S")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'C':
			conn_timeout = atoi(optarg);
			break;
		case 'I':
			rw_timeout = atoi(optarg);
			break;
		case 'a':
			cmd = optarg;
			break;
		case 'S':
			slice_req = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	client.set_slice_request(slice_req);
	acl::redis_connection redis(&client);

	bool ret;

	if (cmd == "auth")
		ret = test_auth(redis);
	else if (cmd == "echo")
		ret = test_echo(redis, n);
	else if (cmd == "ping")
		ret = test_ping(redis, n);
	else if (cmd == "quit")
		ret = test_quit(redis);
	else if (cmd == "select")
		ret = test_select(redis, n);
	else if (cmd == "all")
	{
		ret = test_auth(redis)
			&& test_echo(redis, n)
			&& test_ping(redis, n)
			&& test_select(redis, n)
			&& test_quit(redis);
	}
	else
	{
		printf("unknown cmd: %s\r\n", cmd.c_str());
		ret = false;
	}

	printf("cmd: %s %s\r\n", cmd.c_str(), ret ? "ok" : "failed");

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
