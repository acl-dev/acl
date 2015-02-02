#include "stdafx.h"

static acl::string __channel_prefix("test_channel");

static bool test_subscribe(acl::redis_pubsub& option, int n)
{
	acl::string channel1, channel2;
	int   ret, i;

	for (i = 0; i < n; i++)
	{
		channel1.format("%s_1_%d", __channel_prefix.c_str(), i);
		channel2.format("%s_2_%d", __channel_prefix.c_str(), i);

		option.reset();

		ret = option.subscribe(channel1.c_str(), channel2.c_str(), NULL);
		if (ret <= 0)
		{
			printf("subscribe %s %s error, ret: %d\r\n",
				channel1.c_str(), channel2.c_str(), ret);
			return false;
		}
		else if (i < 10)
			printf("subscribe %s %s ok\r\n", channel1.c_str(),
				channel2.c_str());
	}

	printf(">>>subscribe total: %d\r\n", i * 2);

	acl::string msg;

	for (i = 0; i < n; i++)
	{
		channel1.clear();
		msg.clear();
		option.reset();

		if ((ret = option.get_message(channel1, msg)) < 0)
		{
			printf("get_message error, ret: %d\r\n", ret);
			return false;
		}
		else if (i < 10)
			printf("get one message: %s, channel: %s\r\n",
				msg.c_str(), channel1.c_str());

		channel2.clear();
		msg.clear();
		option.reset();

		if ((ret = option.get_message(channel2, msg)) < 0)
		{
			printf("get_message error, ret: %d\r\n", ret);
			return false;
		}
		else if (i < 10)
			printf("get one message: %s, channel: %s\r\n",
				msg.c_str(), channel2.c_str());

	}

	printf(">>>message total: %d\r\n", i * 2);

	return true;
}

static bool test_publish(acl::redis_pubsub& option, int n)
{
	acl::string channel, msg;
	int   ret, i;

	for (i = 0; i < n; i++)
	{
		channel.format("%s_1_%d", __channel_prefix.c_str(), i);
		msg.format("msg_1_%s", channel.c_str());

		option.reset();
		ret = option.publish(channel.c_str(), msg.c_str(), msg.length());
		if (ret <= 0)
		{
			printf("publish to %s %s error, ret: %d\r\n",
				channel.c_str(), msg.c_str(), ret);
			return false;
		}
		else if (i < 10)
			printf("publish to %s %s ok\r\n", channel.c_str(),
				msg.c_str());

		channel.format("%s_2_%d", __channel_prefix.c_str(), i);
		msg.format("msg_2_%s", channel.c_str());
		option.reset();

		ret = option.publish(channel.c_str(), msg.c_str(), msg.length());
		if (ret <= 0)
		{
			printf("publish to %s %s error, ret: %d\r\n",
				channel.c_str(), msg.c_str(), ret);
			return false;
		}
		else if (i < 10)
			printf("publish to %s %s ok\r\n", channel.c_str(),
				msg.c_str());
	}

	printf(">>>publish total: %d\r\n", i * 2);
	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-n count\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 0]\r\n"
		"-a cmd[subscribe|publish]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 0;
	acl::string addr("127.0.0.1:6379"), cmd;

	while ((ch = getopt(argc, argv, "hs:n:C:I:a:")) > 0)
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
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);
	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	acl::redis_pubsub option(&client);

	bool ret;

	if (cmd == "subscribe")
		ret = test_subscribe(option, n);
	else if (cmd == "publish")
		ret = test_publish(option, n);
	else
	{
		ret = false;
		printf("unknown cmd: %s\r\n", cmd.c_str());
	}

	if (ret == true)
		printf("test OK!\r\n");
	else
		printf("test failed!\r\n");

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
