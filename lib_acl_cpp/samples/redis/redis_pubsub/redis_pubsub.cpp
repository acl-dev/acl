#include "stdafx.h"

static acl::string __channel_prefix("test_channel");
static int __count = 0;
static bool __show = false;

static bool get_msg(acl::redis_pubsub& redis, int cnt)
{
	acl::string channel, msg;

	for (int i = 0; i < cnt; i++) {
		channel.clear();
		msg.clear();
		redis.clear();

		if ((redis.get_message(channel, msg)) == false) {
			printf("get_message error(%s)\r\n", redis.result_error());
			return false;
		}

		if (++__count < 10 || __show) {
			printf("get message from channel %s, msg=%s\r\n",
				channel.c_str(), msg.c_str());
		}
	}

	return true;
}

static bool test_subscribe(acl::redis_pubsub& redis, int n, int cnt)
{
	acl::string channel;

	for (int i = 0; i < n; i++) {
		channel.format("%s_%d", __channel_prefix.c_str(), i);

		redis.clear();
		int ret = redis.subscribe(channel, NULL);
		if (ret <= 0) {
			printf("subscribe %serror(%s), ret: %d\r\n",
				channel.c_str(), redis.result_error(), ret);
			return false;
		} else if (i < 10) {
			printf("subscribe %s ok\r\n", channel.c_str());
		}
	}

	printf(">>>subscribe channels total: %d\r\n", n);

	acl::redis_client* client = redis.get_client();
	if (client) {
		acl::socket_stream* conn = client->get_stream(false);
		const char* peer_addr;
		if (conn) {
			peer_addr = conn->get_peer(true);
		} else {
			peer_addr = "";
		}
		printf("subscribe client on addr=%s, peer=%s\r\n",
			client->get_addr(), peer_addr);
	}

	for (int i = 0; i < n; i++) {
		if (get_msg(redis, cnt) == false) {
			printf("get_msg error\r\n");
			return false;
		}
	}

	printf(">>>message total: %d\r\n", n * cnt);
	return true;
}

static bool pub_msg(acl::redis_pubsub& redis, const char* channel, int n)
{
	acl::string msg;

	for (int i = 0; i < n; i++) {
		redis.clear();
		msg.format("msg_%s_%d", channel, i);

		int ret = redis.publish(channel, msg.c_str(), msg.length());
		if (ret < 0) {
			printf("publish to %s %s error(%s), ret: %d\r\n",
				channel, msg.c_str(),
				redis.result_error(), ret);
			return false;
		}

		if (++__count < 10 || __show) {
			printf("publish ok, channel=%s, msg=%s\r\n",
				channel, msg.c_str());
		}
	}

	return true;
}

static bool test_publish(acl::redis_pubsub& redis, int n, int cnt)
{
	acl::string channel;

	for (int i = 0; i < n; i++) {
		channel.format("%s_%d", __channel_prefix.c_str(), i);
		if (!pub_msg(redis, channel, cnt)) {
			return false;
		}
	}

	printf(">>>publish total: %d\r\n", n * cnt);
	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-p passwd\r\n"
		"-n count\r\n"
		"-m msg_count\r\n"
		"-c connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 0]\r\n"
		"-C [use cluster mode]\r\n"
		"-S [if show messages]\r\n"
		"-a cmd[subscribe|publish]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, cnt = 10, conn_timeout = 10, rw_timeout = 0;
	acl::string addr("127.0.0.1:6379"), cmd, passwd;
	bool cluster_mode = false;

	while ((ch = getopt(argc, argv, "hs:n:m:c:I:a:Cp:S")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'm':
			cnt = atoi(optarg);
			break;
		case 'c':
			conn_timeout = atoi(optarg);
			break;
		case 'I':
			rw_timeout = atoi(optarg);
			break;
		case 'a':
			cmd = optarg;
			break;
		case 'C':
			cluster_mode = true;
			break;
		case 'p':
			passwd = optarg;
			break;
		case 'S':
			__show = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	acl::redis_client_cluster cluster;
	cluster.set(addr.c_str(), 100, conn_timeout, rw_timeout);

	if (!passwd.empty()) {
		client.set_password(passwd);
		cluster.set_password("default", passwd);
	}


	acl::redis_pubsub redis;

	if (cluster_mode) {
		printf("use cluster mode\r\n");
		redis.set_cluster(&cluster);
	} else {
		printf("use single mode\r\n");
		redis.set_client(&client);
	}

	bool ret;

	if (cmd == "subscribe") {
		ret = test_subscribe(redis, n, cnt);
	} else if (cmd == "publish") {
		ret = test_publish(redis, n, cnt);
	} else {
		ret = false;
		printf("unknown cmd: %s\r\n", cmd.c_str());
	}

	if (ret == true) {
		printf("test OK!\r\n");
	} else {
		printf("test failed!\r\n");
	}

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
