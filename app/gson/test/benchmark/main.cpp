#include "stdafx.h"
#include <list>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "struct.h"
#include "struct.gson.h"
#include "util.h"

static void print_msg(message& msg)
{
	printf("type: %d\r\n", msg.type_);
	printf("cmd: %s\r\n", msg.cmd_.c_str());

	size_t i = 0;
	for (auto cit : msg.data_)
	{
		printf(">>username: %s, domain: %s, age: %d, male: %s\r\n",
			cit.username_.c_str(), cit.domain_.c_str(),
			cit.age_, cit.male_ ? "true" : "false");
		if (++i >= 10)
			break;
	}
}

static void print_msg1(message1& msg)
{
	printf("type: %d\r\n", msg.type_);
	printf("cmd: %s\r\n", msg.cmd_.c_str());

	size_t i = 0;
	for (auto cit : msg.data_)
	{
		printf(">>username: %s, domain: %s, age: %d, male: %s\r\n",
			cit->username_, cit->domain_,
			cit->age_, cit->male_ ? "true" : "false");
		if (++i >= 10)
			break;
	}
}

static void test1(void)
{
	message msg;

	msg.type_ = 1;
	msg.cmd_ = "add";

	user user = {"zsx1", "263.net1", 11, true};
	msg.data_.emplace_back(user);

	user = {"zsx2", "263.net2", 12, true};
	msg.data_.emplace_back(user);

	msg.data_.emplace_back("zsx3", "263.net3", 13, true);
	msg.data_.emplace_back("zsx2", "263.net2", 12, false);

	acl::json json;
	acl::json_node& node = acl::gson(json, msg);

	message msg1;
	acl::json json1;
	json1.update(node.to_string());

	printf("------------------------------------------------------\r\n");
	printf("json  to_string: %s\r\n", json.to_string().c_str());
	printf("------------------------------------------------------\r\n");
	printf("node  to_string: %s\r\n", node.to_string().c_str());
	printf("------------------------------------------------------\r\n");
	printf("json1 to_string: %s\r\n", json1.to_string().c_str());
	printf("------------------------------------------------------\r\n");

	std::pair<bool, std::string> ret = acl::gson(json1.get_root(), msg1);
	if (ret.first == false)
		printf("error: %s\r\n", ret.second.c_str());
	else
	{
		printf("---- ok ----\r\n");
		print_msg(msg);
	}
}

static void benchmark(int max)
{
	message msg;
	msg.type_ = 10;
	msg.cmd_ = "add";

	struct timeval begin;
	gettimeofday(&begin, NULL);

	for (int i = 0; i < max; i++)
		msg.data_.emplace_back("zsx", "263.net", 11, true);

	acl::json json;
	acl::json_node& node = acl::gson(json, msg);

	struct timeval end;
	gettimeofday(&end, NULL);
	double spent = util::stamp_sub(&end, &begin);
	printf("emplace_back, struct --> json spent: %.2f ms, count: %d, speed: %.2f\r\n",
		spent, max, (max * 1000) / (spent == 0 ? 1 : spent));

	printf("------------------------------------------------------\r\n");

	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();

	acl::json json1;

	gettimeofday(&begin, NULL);
	const acl::string& buf = node.to_string();
	gettimeofday(&end, NULL);
	spent = util::stamp_sub(&end, &begin);
	printf("json to_string spent: %.2f ms, count: %d, speed: %.2f\r\n",
		spent, max, (max * 1000) / (spent == 0 ? 1 : spent));

	gettimeofday(&begin, NULL);

	json1.update(buf);

	gettimeofday(&end, NULL);
	spent = util::stamp_sub(&end, &begin);
	printf("json parsing spent: %.2f ms, count: %d, speed: %.2f\r\n",
		spent, max, (max * 1000) / (spent == 0 ? 1 : spent));

	printf("------------------------------------------------------\r\n");

	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();

	message msg1;

	gettimeofday(&begin, NULL);

	std::pair<bool, std::string> res = acl::gson(json1.get_root(), msg1);

	gettimeofday(&end, NULL);

	spent = util::stamp_sub(&end, &begin);
	printf("json --> struct spent: %.2f ms, count: %d, speed: %.2f\r\n",
		spent, max, (max * 1000) / (spent == 0 ? 1 : spent));

	printf("------------------------------------------------------\r\n");

	if (res.first == false)
		printf("error: %s\r\n", res.second.c_str());
	else
	{
		print_msg(msg1);
		printf("------------------- ok --------------------\r\n");
	}
}

static void test_mem(int max)
{
	message1* msg = new message1;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	msg->type_ = 100;
	msg->cmd_ = "add";

	for (int i = 0; i < max; i++)
	{
		user1* user = new user1("zsx", "263.net", 11, true);
		msg->data_.push_back(user);
	}

	acl::json json;
	acl::json_node& node = acl::gson(json, msg);

	struct timeval end;
	gettimeofday(&end, NULL);
	double spent = util::stamp_sub(&end, &begin);
	printf("push_back, struct --> json spent: %.2f ms, count: %d, speed: %.2f\r\n",
		spent, max, (max * 1000) / (spent == 0 ? 1 : spent));

	delete msg;

	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();

	//////////////////////////////////////////////////////////////////////
	
	acl::json json1;
	json1.update(node.to_string());

	msg = new message1;
	gettimeofday(&begin, NULL);

	std::pair<bool, std::string> res = acl::gson(json1.get_root(), msg);

	gettimeofday(&end, NULL);

	spent = util::stamp_sub(&end, &begin);

	printf("json --> struct spent: %.2f ms, count: %d, speed: %.2f\r\n",
		spent, max, (max * 1000) / (spent == 0 ? 1 : spent));

	printf("------------------------------------------------------\r\n");

	if (res.first == false)
		printf("error: %s\r\n", res.second.c_str());
	else
	{
		print_msg1(*msg);
		printf("------------------- ok --------------------\r\n");
	}

	delete msg;
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -n max_count\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch, max = 10000;

	while ((ch = getopt(argc, argv, "hn:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max = atoi(optarg);
			break;
		default:
			break;
		}
	}

	test1();
	benchmark(max);
	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();

	test_mem(max);

	printf("Enter any key to exit ...");
	fflush(stdout);
	getchar();

	return 0;
}
