#include "stdafx.h"
#include <list>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "struct.h"
#include "struct.gson.h"

static void print_msg(message& msg)
{
	printf("=======================================================\r\n");
	printf("type: %d\r\n", msg.type);
	printf("cmd: %s\r\n", msg.cmd.c_str());

	printf("-------------------- user list ------------------------\r\n");
	size_t i = 0;
	for (std::list<user>::const_iterator cit = msg.user_list.begin();
		cit != msg.user_list.end(); ++cit)
	{
		printf(">>username: %s, domain: %s, age: %d, male: %s\r\n",
			(*cit).username.c_str(), (*cit).domain.c_str(),
			(*cit).age, (*cit).male ? "true" : "false");
		if (++i >= 10)
			break;
	}

	printf("-------------------- user vector ----------------------\r\n");
	i = 0;
	for (std::vector<user>::const_iterator cit = msg.user_vector.begin();
		cit != msg.user_vector.end(); ++cit)
	{
		printf(">>username: %s, domain: %s, age: %d, male: %s\r\n",
			(*cit).username.c_str(), (*cit).domain.c_str(),
			(*cit).age, (*cit).male ? "true" : "false");
		if (++i >= 10)
			break;
	}

	printf("------------------- user map --------------------------\r\n");
	i = 0;
	for (std::map<acl::string, user>::const_iterator cit = msg.user_map.begin();
		cit != msg.user_map.end(); ++cit)
	{
		printf(">>key: %s, username: %s, domain: %s, age: %d, male: %s\r\n",
			cit->first.c_str(),
			cit->second.username.c_str(),
			cit->second.domain.c_str(),
			cit->second.age,
			cit->second.male ? "true" : "false");
		if (++i >= 10)
			break;
	}
	printf("-------------------- user list ptr --------------------\r\n");
	i = 0;
	for (std::list<user*>::const_iterator cit = msg.user_list_ptr->begin();
		cit != msg.user_list_ptr->end(); ++cit)
	{
		printf(">>username: %s, domain: %s, age: %d, male: %s\r\n",
			(*cit)->username.c_str(), (*cit)->domain.c_str(),
			(*cit)->age, (*cit)->male ? "true" : "false");
		if (++i >= 10)
			break;
	}

	printf("-------------------- user vector ptr ------------------\r\n");
	i = 0;
	for (std::vector<user*>::const_iterator cit = msg.user_vector_ptr->begin();
		cit != msg.user_vector_ptr->end(); ++cit)
	{
		printf(">>username: %s, domain: %s, age: %d, male: %s\r\n",
			(*cit)->username.c_str(), (*cit)->domain.c_str(),
			(*cit)->age, (*cit)->male ? "true" : "false");
		if (++i >= 10)
			break;
	}

	printf("------------------- user map ptr ----------------------\r\n");
	i = 0;
	for (std::map<acl::string, user*>::const_iterator cit =
		msg.user_map_ptr->begin(); cit != msg.user_map_ptr->end(); ++cit)
	{
		printf(">>key: %s, username: %s, domain: %s, age: %d, male: %s\r\n",
			cit->first.c_str(),
			cit->second->username.c_str(),
			cit->second->domain.c_str(),
			cit->second->age,
			cit->second->male ? "true" : "false");
		if (++i >= 10)
			break;
	}

	printf("-------------------------------------------------------\r\n");
}

static void test1(void)
{
	message msg;
	msg.user_list_ptr   = new std::list<user*>;
	msg.user_vector_ptr = new std::vector<user*>;
	msg.user_map_ptr    = new std::map<acl::string, user*>;

	msg.type = 1;
	msg.cmd  = "add";

	user u("zsx1", "263.net", 11, true);
	msg.user_list.push_back(u);

	u.username = "zsx2";
	u.domain = "263.net";
	u.age = 11;
	u.male = true;
	msg.user_vector.push_back(u);

	u.username = "zsx31";
	u.domain = "263.net";
	u.age = 11;
	u.male = true;
	msg.user_map[u.username] = u;

	u.username = "zsx32";
	u.domain = "263.net";
	u.age = 11;
	u.male = true;
	msg.user_map["zsx32"] = u;

	msg.user_list_ptr->push_back(new user("zsx4", "263.net1", 11, true));
	msg.user_list_ptr->push_back(new user("zsx4", "263.net2", 12, true));

	msg.user_vector_ptr->push_back(new user("zsx5", "263.net1", 11, true));
	msg.user_vector_ptr->push_back(new user("zsx5", "263.net2", 12, true));

	(*msg.user_map_ptr)["zsx61"] = new user("zsx61:", "263.net1", 11, true);
	(*msg.user_map_ptr)["zsx62"] = new user("zsx62", "263.net2", 12, true);

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
		printf("==================== All OK ===================\r\n");
		print_msg(msg);
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch;

	while ((ch = getopt(argc, argv, "h")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}

	test1();
	printf("Enter any key to continue ..."); fflush(stdout); getchar();
	return 0;
}
