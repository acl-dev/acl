#include "stdafx.h"
#include "redis_util.h"

redis_util::redis_util(void)
{
}

redis_util::~redis_util(void)
{
}

bool redis_util::get_node_id(const char* addr, acl::string& node_id)
{
	acl::redis_client client(addr);
	acl::redis redis(&client);
	return get_node_id(redis, node_id);
}

bool redis_util::get_node_id(acl::redis& redis, acl::string& node_id)
{
	acl::socket_stream* conn = redis.get_client()->get_stream();
	if (conn == NULL)
	{
		printf("%s: connection disconnected!\r\n", __FUNCTION__);
		return false;
	}

	const char* addr = conn->get_peer(true);

	const std::map<acl::string, acl::redis_node*>* nodes =
		redis.cluster_nodes();
	if (nodes == NULL)
	{
		printf("%s: cluster_nodes null, addr: %s\r\n",
			__FUNCTION__, addr);
		return false;
	}

	std::map<acl::string, acl::redis_node*>::const_iterator it;

	for (it = nodes->begin(); it != nodes->end(); ++it)
	{
		const acl::redis_node* node = it->second;
		if (node->is_myself())
		{
			node_id = node->get_id();
			if (node_id.empty())
				return false;
			return true;
		}
	}

	printf("cluster_nodes no myself id, addr: %s\r\n", addr);

	return false;
}

bool redis_util::get_ip(const char* addr, acl::string& buf)
{
	acl::string tmp(addr);
	const std::vector<acl::string>& tokens = tmp.split2(":");
	if (tokens.size() != 2)
	{
		printf("%s: invalid addr: %s\r\n", __FUNCTION__, addr);
		return false;
	}

	buf = tokens[0].c_str();
	return true;
}

void redis_util::free_nodes(const std::vector<acl::redis_node*>& nodes)
{
	std::vector<acl::redis_node*>::const_iterator it;
	for (it = nodes.begin(); it != nodes.end(); ++it)
	{
		const std::vector<acl::redis_node*>* slaves =
			(*it)->get_slaves();
		if (!slaves->empty())
			free_nodes(*slaves);
		delete *it;
	}
}

void redis_util::print_nodes(int nested,
	const std::vector<acl::redis_node*>& nodes)
{
	nested++;
	std::vector<acl::redis_node*>::const_iterator cit;
	for (cit = nodes.begin(); cit != nodes.end(); ++cit)
	{
		for (int i = 0; i < nested - 1; ++i)
			printf("\t");
		printf("addr: %s\r\n", (*cit)->get_addr());
		const std::vector<acl::redis_node*>* slaves =
			(*cit)->get_slaves();
		if (!slaves->empty())
			print_nodes(nested, *slaves);
	}
}
