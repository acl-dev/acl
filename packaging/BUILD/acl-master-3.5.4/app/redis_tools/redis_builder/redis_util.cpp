#include "stdafx.h"
#include "redis_util.h"

redis_util::redis_util(void)
{
}

redis_util::~redis_util(void)
{
}

bool redis_util::get_node_id(const char* addr, acl::string& node_id,
	const char* passwd)
{
	acl::redis_client client(addr);
	if (passwd && *passwd) {
		client.set_password(passwd);
	}
	acl::redis redis(&client);
	return get_node_id(redis, node_id);
}

bool redis_util::get_node_id(acl::redis& redis, acl::string& node_id)
{
	acl::socket_stream* conn = redis.get_client()->get_stream();
	if (conn == NULL) {
		logger_error("%s: connection disconnected!", __FUNCTION__);
		return false;
	}

	const char* addr = conn->get_peer(true);

	const std::map<acl::string, acl::redis_node*>* nodes =
		redis.cluster_nodes();
	if (nodes == NULL) {
		logger_error("%s: cluster_nodes null, addr: %s",
			__FUNCTION__, addr);
		return false;
	}

	std::map<acl::string, acl::redis_node*>::const_iterator it;

	for (it = nodes->begin(); it != nodes->end(); ++it) {
		const acl::redis_node* node = it->second;
		if (node->is_myself()) {
			node_id = node->get_id();
			if (node_id.empty()) {
				return false;
			}
			return true;
		}
	}

	logger_error("cluster_nodes no myself id, addr: %s", addr);

	return false;
}

bool redis_util::get_ip(const char* addr, acl::string& ip)
{
	acl::string buf(addr);
	const std::vector<acl::string>& tokens = buf.split2(":");
	if (tokens.size() != 2) {
		logger_error("%s: invalid addr: %s", __FUNCTION__, addr);
		return false;
	}

	ip = tokens[0].c_str();
	return true;
}

bool redis_util::addr_split(const char* addr, acl::string& ip, int& port)
{
	acl::string buf(addr);
	const std::vector<acl::string>& tokens = buf.split2(":");
	if (tokens.size() != 2) {
		logger_error("%s: invalid addr: %s", __FUNCTION__, addr);
		return false;
	}

	ip = tokens[0];
	port = atoi(tokens[1].c_str());
	return true;
}

void redis_util::free_nodes(const std::vector<acl::redis_node*>& nodes)
{
	std::vector<acl::redis_node*>::const_iterator it;
	for (it = nodes.begin(); it != nodes.end(); ++it) {
		const std::vector<acl::redis_node*>* slaves =
			(*it)->get_slaves();
		if (!slaves->empty()) {
			free_nodes(*slaves);
		}
		delete *it;
	}
}

void redis_util::print_nodes(int nested,
	const std::vector<acl::redis_node*>& nodes)
{
	nested++;
	std::vector<acl::redis_node*>::const_iterator cit;
	for (cit = nodes.begin(); cit != nodes.end(); ++cit) {
		for (int i = 0; i < nested - 1; ++i) {
			printf("\t");
		}
		printf("addr: %s\r\n", (*cit)->get_addr());
		const std::vector<acl::redis_node*>* slaves =
			(*cit)->get_slaves();
		if (!slaves->empty()) {
			print_nodes(nested, *slaves);
		}
	}
}

void redis_util::clear_nodes_container(
	std::map<acl::string, std::vector<acl::redis_node*>* >& nodes)
{
	for (std::map<acl::string, std::vector<acl::redis_node*>* >
		::const_iterator cit = nodes.begin();
		cit != nodes.end(); ++cit) {

		delete cit->second;
	}

	nodes.clear();
}

void redis_util::sort(const std::map<acl::string, acl::redis_node*>& in,
	std::map<acl::string, std::vector<acl::redis_node*>* >& out)
{
	acl::string ip;
	int port;

	for (std::map<acl::string, acl::redis_node*>::const_iterator cit =
		in.begin(); cit != in.end(); ++cit) {

		if (!redis_util::addr_split(cit->second->get_addr(), ip, port)) {
			logger_error("invalid addr: %s",
				cit->second->get_addr());
			continue;
		}

		std::map<acl::string, std::vector<acl::redis_node*>* >
			::const_iterator cit_node = out.find(ip);
		if (cit_node == out.end()) {
			std::vector<acl::redis_node*>* a = new
				std::vector<acl::redis_node*>;
			a->push_back(cit->second);
			out[ip] = a;
		} else {
			cit_node->second->push_back(cit->second);
		}
	}
}

void redis_util::get_nodes(acl::redis& redis, bool prefer_master,
	std::vector<acl::redis_node*>& nodes)
{
	const std::map<acl::string, acl::redis_node*>* masters
		= get_masters(redis);
	if (masters == NULL) {
		logger_error("get_masters NULL");
		return;
	}

	if (prefer_master) {
		for (std::map<acl::string, acl::redis_node*>::const_iterator
			cit = masters->begin(); cit != masters->end(); ++cit) {

			nodes.push_back(cit->second);
		}

		return;
	}

	const std::vector<acl::redis_node*>* slaves;

	for (std::map<acl::string, acl::redis_node*>::const_iterator cit
		= masters->begin(); cit != masters->end(); ++cit) {

		slaves = cit->second->get_slaves();
		if (slaves != NULL && !slaves->empty()) {
			nodes.push_back((*slaves)[0]);
		} else {
			nodes.push_back(cit->second);
		}
	}
}

const std::map<acl::string, acl::redis_node*>* redis_util::get_masters(
	acl::redis& redis)
{
	const std::map<acl::string, acl::redis_node*>* masters =
		get_masters2(redis);
	if (masters != NULL) {
		return masters;
	}

	acl::redis_client* conn = redis.get_client();
	if (conn == NULL) {
		return NULL;
	}

	const char* addr = conn->get_addr();
	if (addr == NULL || *addr == 0) {
		logger_error("get_addr NULL");
		return NULL;
	}

	static std::map<acl::string, acl::redis_node*> single_master_;

	for (std::map<acl::string, acl::redis_node*>::iterator it
		= single_master_.begin(); it != single_master_.end(); ++it) {

		delete it->second;
	}
	single_master_.clear();

	acl::redis_node* node = new acl::redis_node;
	node->set_addr(addr);
	single_master_[addr] = node;
	return &single_master_;
}

const std::map<acl::string, acl::redis_node*>* redis_util::get_masters2(
	acl::redis& redis)
{
	std::map<acl::string, acl::string> res;
	if (redis.info(res) <= 0) {
		logger_error("redis.info error: %s", redis.result_error());
		return NULL;
	}

	const char* name = "cluster_enabled";
	std::map<acl::string, acl::string>::const_iterator cit = res.find(name);
	if (cit == res.end()) {
		logger("no cluster_enabled");
		return NULL;
	}
	if (!cit->second.equal("1")) {
		logger("cluster_enabled: %s", cit->second.c_str());
		return NULL;
	}

	const std::map<acl::string, acl::redis_node*>* masters =
		redis.cluster_nodes();
	if (masters == NULL) {
		logger_error("masters NULL");
	}

	return masters;
}

void redis_util::get_all_nodes(acl::redis& redis,
	std::vector<const acl::redis_node*>& nodes)
{
	const std::map<acl::string, acl::redis_node*>* masters
		= get_masters(redis);
	if (masters == NULL) {
		logger_error("get_masters NULL");
		return;
	}

	for (std::map<acl::string, acl::redis_node*>::const_iterator cit
		= masters->begin(); cit != masters->end(); ++cit) {

		nodes.push_back(cit->second);
		add_slaves(cit->second, nodes);
	}
}

void redis_util::get_slaves(acl::redis& redis,
	std::vector<const acl::redis_node*>& nodes)
{
	const std::map<acl::string, acl::redis_node*>* masters
		= get_masters(redis);
	if (masters == NULL) {
		return;
	}

	for (std::map<acl::string, acl::redis_node*>::const_iterator cit
		= masters->begin(); cit != masters->end(); ++cit) {

		add_slaves(cit->second, nodes);
	}
}

void redis_util::add_slaves(const acl::redis_node* node,
	std::vector<const acl::redis_node*>& nodes)
{
	if (node == NULL) {
		return;
	}

	const std::vector<acl::redis_node*>* slaves = node->get_slaves();
	if (slaves == NULL) {
		return;
	}

	for (std::vector<acl::redis_node*>::const_iterator cit
		= slaves->begin(); cit != slaves->end(); ++cit) {

		nodes.push_back(*cit);
		add_slaves(*cit, nodes);
	}
}
