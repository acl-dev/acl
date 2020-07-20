#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/redis/redis_node.hpp"
#include "acl_cpp/redis/redis_slot.hpp"
#include "acl_cpp/redis/redis_cluster.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

#define INT_LEN		11
#define LONG_LEN	21

redis_cluster::redis_cluster()
{
}

redis_cluster::redis_cluster(redis_client* conn)
: redis_command(conn)
{
}

redis_cluster::redis_cluster(redis_client_cluster* cluster)
: redis_command(cluster)
{
}

redis_cluster::redis_cluster(redis_client_cluster* cluster, size_t)
: redis_command(cluster)
{
}

redis_cluster::~redis_cluster()
{
	free_slots();
	free_masters();
	free_slaves();
}

bool redis_cluster::cluster_addslots(int first, ...)
{
	std::vector<int> slot_list;
	va_list ap;
	va_start(ap, first);
	int  n;
	while ((n = va_arg(ap, int)) >= 0)
		slot_list.push_back(n);
	va_end(ap);

	if (slot_list.empty())
		return true;
	return cluster_addslots(slot_list);
}

bool redis_cluster::cluster_addslots(const int slot_list[], size_t n)
{
	build("CLUSTER", "ADDSLOTS", slot_list, n);
	return check_status();
}

bool redis_cluster::cluster_addslots(const std::vector<int>& slot_list)
{
	build("CLUSTER", "ADDSLOTS", slot_list);
	return check_status();
}

bool redis_cluster::cluster_delslots(int first, ...)
{
	std::vector<int> slot_list;
	va_list ap;
	va_start(ap, first);
	int  n;
	while ((n = va_arg(ap, int)) >= 0)
		slot_list.push_back(n);
	va_end(ap);

	if (slot_list.empty())
		return true;
	return cluster_delslots(slot_list);
}

bool redis_cluster::cluster_delslots(const int slot_list[], size_t n)
{
	build("CLUSTER", "DELSLOTS", slot_list, n);
	return check_status();
}

bool redis_cluster::cluster_delslots(const std::vector<int>& slot_list)
{
	build("CLUSTER", "DELSLOTS", slot_list);
	return check_status();
}

int redis_cluster::cluster_getkeysinslot(size_t slot, size_t max,
	std::list<string>& result)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "GETKEYSINSLOT";
	lens[1] = sizeof("GETKEYSINSLOT") - 1;

	char slot_s[LONG_LEN];
	safe_snprintf(slot_s, sizeof(slot_s), "%lu", (unsigned long) slot);
	argv[2] = slot_s;
	lens[2] = strlen(slot_s);

	char max_s[LONG_LEN];
	safe_snprintf(max_s, sizeof(max_s), "%lu", (unsigned long) max);
	argv[3] = max_s;
	lens[3] = strlen(max_s);

	build_request(4, argv, lens);
	return get_strings(result);
}

bool redis_cluster::cluster_meet(const char* ip, int port)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "MEET";
	lens[1] = sizeof("MEET") - 1;

	argv[2] = ip;
	lens[2] = strlen(ip);

	char port_s[INT_LEN];
	safe_snprintf(port_s, sizeof(port_s), "%d", port);
	argv[3] = port_s;
	lens[3] = strlen(port_s);

	build_request(4, argv, lens);
	return check_status();
}

bool redis_cluster::cluster_reset()
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "RESET";
	lens[1] = sizeof("RESET") - 1;

	build_request(2, argv, lens);
	return check_status();
}

bool redis_cluster::cluster_reset_hard()
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "RESET";
	lens[1] = sizeof("RESET") - 1;

	argv[2] = "HARD";
	lens[2] = sizeof("HARD") - 1;

	build_request(3, argv, lens);
	return check_status();

}

bool redis_cluster::cluster_reset_soft()
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "RESET";
	lens[1] = sizeof("RESET") - 1;

	argv[2] = "SOFT";
	lens[2] = sizeof("SOFT") - 1;

	build_request(3, argv, lens);
	return check_status();
}

bool redis_cluster::cluster_setslot_importing(size_t slot, const char* src_node)
{
	const char* argv[5];
	size_t lens[5];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "SETSLOT";
	lens[1] = sizeof("SETSLOT") - 1;

	char slot_s[LONG_LEN];
	safe_snprintf(slot_s, sizeof(slot_s), "%lu", (unsigned long) slot);
	argv[2] = slot_s;
	lens[2] = strlen(slot_s);

	argv[3] = "IMPORTING";
	lens[3] = sizeof("IMPORTING") - 1;

	argv[4] = src_node;
	lens[4] = strlen(src_node);

	build_request(5, argv, lens);
	return check_status();
}

bool redis_cluster::cluster_setslot_migrating(size_t slot, const char* dst_node)
{
	const char* argv[5];
	size_t lens[5];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "SETSLOT";
	lens[1] = sizeof("SETSLOT") - 1;

	char slot_s[LONG_LEN];
	safe_snprintf(slot_s, sizeof(slot_s), "%lu", (unsigned long) slot);
	argv[2] = slot_s;
	lens[2] = strlen(slot_s);

	argv[3] = "MIGRATING";
	lens[3] = sizeof("MIGRATING") - 1;

	argv[4] = dst_node;
	lens[4] = strlen(dst_node);

	build_request(5, argv, lens);
	return check_status();
}

bool redis_cluster::cluster_setslot_stable(size_t slot)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "SETSLOT";
	lens[1] = sizeof("SETSLOT") - 1;

	char slot_s[LONG_LEN];
	safe_snprintf(slot_s, sizeof(slot_s), "%lu", (unsigned long) slot);
	argv[2] = slot_s;
	lens[2] = strlen(slot_s);

	argv[3] = "STABLE";
	lens[3] = sizeof("STABLE") - 1;

	build_request(4, argv, lens);
	return check_status();
}

bool redis_cluster::cluster_setslot_node(size_t slot, const char* node)
{
	const char* argv[5];
	size_t lens[5];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "SETSLOT";
	lens[1] = sizeof("SETSLOT") - 1;

	char slot_s[LONG_LEN];
	safe_snprintf(slot_s, sizeof(slot_s), "%lu", (unsigned long) slot);
	argv[2] = slot_s;
	lens[2] = strlen(slot_s);

	argv[3] = "NODE";
	lens[3] = sizeof("NODE") - 1;

	argv[4] = node;
	lens[4] = strlen(node);

	build_request(5, argv, lens);
	return check_status();
}

int redis_cluster::cluster_count_failure_reports(const char* node)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "COUNT-FAILURE-REPORTS";
	lens[1] = sizeof("COUNT-FAILURE-REPORTS") - 1;

	argv[2] = node;
	lens[2] = strlen(node);

	build_request(3, argv, lens);
	return get_number();
}

bool redis_cluster::cluster_failover()
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "FAILOVER";
	lens[1] = sizeof("FAILOVER") - 1;

	build_request(2, argv, lens);
	return check_status();
}

bool redis_cluster::cluster_failover_force()
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "FAILOVER";
	lens[1] = sizeof("FAILOVER") - 1;

	argv[2] = "FORCE";
	lens[2] = sizeof("FORCE") - 1;

	build_request(3, argv, lens);
	return check_status();
}

bool redis_cluster::cluster_failover_takeover()
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "FAILOVER";
	lens[1] = sizeof("FAILOVER") - 1;

	argv[2] = "TAKEOVER";
	lens[2] = sizeof("TAKEOVER") - 1;

	build_request(3, argv, lens);
	return check_status();
}

bool redis_cluster::cluster_info(std::map<string, string>& result)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "INFO";
	lens[1] = sizeof("INFO") - 1;

	build_request(2, argv, lens);

	string buf;
	if (get_string(buf) <= 0)
		return false;

	string line;

	while (true) {
		line.clear();
		if (buf.scan_line(line) == false)
			break;

		char* name = line.c_str();
		char* value = strchr(name, ':');
		if (value == NULL || *(value + 1) == 0)
			continue;
		*value++ = 0;
		result[name] = value;
	}

	return true;
}

bool redis_cluster::cluster_saveconfig()
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "SAVECONFIG";
	lens[1] = sizeof("SAVECONFIG") - 1;

	build_request(2, argv, lens);
	return check_status();
}

int redis_cluster::cluster_countkeysinslot(size_t slot)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "COUNTKEYSINSLOT";
	lens[1] = sizeof("COUNTKEYSINSLOT") - 1;

	char slot_s[LONG_LEN];
	safe_snprintf(slot_s, sizeof(slot_s), "%lu", (unsigned long) slot);
	argv[2] = slot_s;
	lens[2] = strlen(slot_s);

	build_request(3, argv, lens);
	return get_number();
}

bool redis_cluster::cluster_forget(const char* node)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "FORGET";
	lens[1] = sizeof("FORGET") - 1;

	argv[2] = node;
	lens[2] = strlen(node);

	build_request(3, argv, lens);
	return check_status();
}

int redis_cluster::cluster_keyslot(const char* key)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "KEYSLOT";
	lens[1] = sizeof("KEYSLOT") - 1;

	argv[2] = key;
	lens[2] = strlen(key);

	build_request(3, argv, lens);
	return get_number();
}

bool redis_cluster::cluster_replicate(const char* node)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "REPLICATE";
	lens[1] = sizeof("REPLICATE") - 1;

	argv[2] = node;
	lens[2] = strlen(node);

	build_request(3, argv, lens);
	return check_status();
}

bool redis_cluster::cluster_set_config_epoch(const char* epoch)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSETR";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "SET-CONFIG-EPOCH";
	lens[1] = sizeof("SET-CONFIG-EPOCH") - 1;

	argv[2] = epoch;
	lens[2] = strlen(epoch);

	build_request(3, argv, lens);
	return check_status();
}

//////////////////////////////////////////////////////////////////////////

const std::vector<redis_slot*>* redis_cluster::cluster_slots()
{
	free_slots();

	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "SLOTS";
	lens[1] = sizeof("SLOTS") - 1;

	build_request(2, argv, lens);
	const redis_result* rr = run();
	if (rr == NULL || rr->get_type() != REDIS_RESULT_ARRAY)
		return NULL;

	size_t size;
	const redis_result** children = rr->get_children(&size);
	if (children == NULL)
		return NULL;

	for (size_t i = 0; i < size; i++) {
		const redis_result* rr2 = children[i];
		if (rr2 == NULL || rr2->get_type() != REDIS_RESULT_ARRAY)
			continue;
		redis_slot* master = get_slot_master(rr2);
		if (master != NULL)
			slots_.push_back(master);
	}

	return &slots_;
}

redis_slot* redis_cluster::get_slot_master(const redis_result* rr)
{
	size_t size;
	const redis_result** children = rr->get_children(&size);
	if (children == NULL)
		return NULL;
	if (size < 3)
		return NULL;

	int slot_min = children[0]->get_integer();
	if (slot_min < 0)
		return NULL;
	int slot_max = children[1]->get_integer();
	if (slot_max < slot_min)
		return NULL;

	redis_slot* master = get_slot(children[2], slot_max, slot_min);
	if (master == NULL)
		return NULL;

	for (size_t i = 3; i < size; i++) {
		redis_slot* slave = get_slot(children[i], slot_max, slot_min);
		if (slave != NULL)
			master->add_slave(slave);
	}

	return master;
}

redis_slot* redis_cluster::get_slot(const redis_result* rr,
	size_t slot_max, size_t slot_min)
{
	if (rr == NULL)
		return NULL;

	size_t size;
	const redis_result** children = rr->get_children(&size);
	if (children == NULL || size < 2)
		return NULL;

	char ip[128];
	if (children[0]->argv_to_string(ip, sizeof(ip)) <= 0)
		return NULL;

	int port = children[1]->get_integer();
	if (port <= 0)
		return NULL;

	redis_slot* slot = NEW redis_slot(slot_min, slot_max, ip, port);
	return slot;
}

void redis_cluster::free_slots()
{
	std::vector<redis_slot*>::iterator it;
	for (it = slots_.begin(); it != slots_.end(); ++it)
		delete *it;
	slots_.clear();
}

//////////////////////////////////////////////////////////////////////////

const std::vector<redis_node*>* redis_cluster::cluster_slaves(const char* node)
{
	free_slaves();

	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "SLAVES";
	lens[1] = sizeof("SLAVES") - 1;

	argv[2] = node;
	lens[2] = strlen(node);

	build_request(3, argv, lens);

	std::vector<string> lines;
	if (get_strings(lines) < 0)
		return NULL;

	std::vector<string>::iterator it = lines.begin();
	for (; it != lines.end(); ++it) {
		std::vector<string>& tokens = (*it).split2(" ");
		if (tokens.size() < 3)
			continue;

		char* node_type = tokens[2].c_str();
		char* ptr = strchr(node_type, ',');
		if (ptr != NULL && *(ptr + 1))
			node_type = ptr + 1;
		if (strcasecmp(node_type, "slave") != 0)
			continue;
		redis_node* slave = get_slave(tokens);
		if (slave != NULL)
			slaves_.push_back(slave);
	}

	return &slaves_;
}

redis_node* redis_cluster::get_slave(const std::vector<string>& tokens)
{
	if (tokens.size() < 8)
		return NULL;

	redis_node* node = NEW redis_node;
	node->set_id(tokens[0].c_str());
	node->set_addr(tokens[1].c_str());
	node->set_myself(false);
	node->set_connected(strcasecmp(tokens[7].c_str(), "connected") == 0);
	node->set_master_id(tokens[3].c_str());
	node->set_type("slave");
	return node;
}

void redis_cluster::free_slaves()
{
	std::vector<redis_node*>::iterator it = slaves_.begin();
	for (; it != slaves_.end(); ++it)
		delete *it;

	slaves_.clear();
}

const std::map<string, redis_node*>* redis_cluster::cluster_nodes()
{
	free_masters();

	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "NODES";
	lens[1] = sizeof("NODES") - 1;

	build_request(2, argv, lens);

	string buf;
	if (get_string(buf) <= 0)
		return NULL;

	std::vector<redis_node*> slaves;
	acl::string line;

	while (true) {
		if (buf.scan_line(line) == false)
			break;
		redis_node* node = get_node(line);
		if (node != NULL && !node->is_master())
			slaves.push_back(node);
		line.clear();
	}

	for (std::vector<redis_node*>::iterator it = slaves.begin();
		it != slaves.end(); ++it) {
		const char* id = (*it)->get_master_id();
		std::map<string, redis_node*>::iterator it2 = masters_.find(id);
		if (it2 != masters_.end())
			it2->second->add_slave(*it);
		else {
			logger_warn("delete orphan slave: %s", id);
			delete *it;
		}
	}

	return &masters_;
}

// for redis.3.x.x
// d52ea3cb4cdf7294ac1fb61c696ae6483377bcfc 127.0.0.1:16385 master - 0 1428410625374 73 connected 5461-10922
// 94e5d32cbcc9539cc1539078ca372094c14f9f49 127.0.0.1:16380 myself,master - 0 0 1 connected 0-9 11-5460
// e7b21f65e8d0d6e82dee026de29e499bb518db36 127.0.0.1:16381 slave d52ea3cb4cdf7294ac1fb61c696ae6483377bcfc 0 1428410625373 73 connected
// 6a78b47b2e150693fc2bed8578a7ca88b8f1e04c 127.0.0.1:16383 myself,slave 94e5d32cbcc9539cc1539078ca372094c14f9f49 0 0 4 connected

// 70a2cd8936a3d28d94b4915afd94ea69a596376a :16381 myself,master - 0 0 0 connected

// for redis.4.x.x
// d52ea3cb4cdf7294ac1fb61c696ae6483377bcfc 127.0.0.1:16385@116385 master - 0 1428410625374 73 connected 5461-10922
// 94e5d32cbcc9539cc1539078ca372094c14f9f49 127.0.0.1:16380@116380 myself,master - 0 0 1 connected 0-9 11-5460
// e7b21f65e8d0d6e82dee026de29e499bb518db36 127.0.0.1:16381@116381 slave d52ea3cb4cdf7294ac1fb61c696ae6483377bcfc 0 1428410625373 73 connected
// 6a78b47b2e150693fc2bed8578a7ca88b8f1e04c 127.0.0.1:16383@116383 myself,slave 94e5d32cbcc9539cc1539078ca372094c14f9f49 0 0 4 connected

// 70a2cd8936a3d28d94b4915afd94ea69a596376a :16381 myself,master - 0 0 0 connected


redis_node* redis_cluster::get_node(string& line)
{
	std::vector<string>& tokens = line.split2(" ");
	if (tokens.size() < 8) {
		logger_warn("invalid tokens's size: %d < 8",
			(int) tokens.size());
		return NULL;
	}

	bool myself = false;
	char* node_type = tokens[2].c_str();
	char* ptr = strchr(node_type, ',');
	if (ptr != NULL && *(ptr + 1) != 0) {
		*ptr++ = 0;
		if (strcasecmp(node_type, "myself") == 0)
			myself = true;
		node_type = ptr;
	}

	redis_node* node = NEW redis_node;
	node->set_id(tokens[0].c_str());
	node->set_addr(tokens[1].c_str());
	node->set_myself(myself);
	node->set_connected(strcasecmp(tokens[7].c_str(), "connected") == 0);
	node->set_master_id(tokens[3].c_str());

	if (strcasecmp(node_type, "master") == 0) {
		node->set_master(node);
		node->set_type("master");
		masters_[tokens[0]] = node;
		size_t n = tokens.size();
		for (size_t i = 8; i < n; i++)
			add_slot_range(node, tokens[i].c_str());
	}
	else if (strcasecmp(node_type, "slave") == 0)
		node->set_type("slave");
	else if (strcasecmp(node_type, "handshake") == 0) {
		node->set_master(node);
		node->set_type("handshake");
		node->set_handshaking(true);
		masters_[tokens[0]] = node;
		size_t n = tokens.size();
		for (size_t i = 8; i < n; i++)
			add_slot_range(node, tokens[i].c_str());
	}
	else
		logger_warn("unknown node type: %s", node_type);

	return node;
}

void redis_cluster::add_slot_range(redis_node* node, char* slots)
{
	size_t slot_min, slot_max;

	char* ptr = strchr(slots, '-');
	if (ptr != NULL && *(ptr + 1) != 0) {
		*ptr++ = 0;
		slot_min = (size_t) atol(slots);
		slot_max = (size_t) atol(ptr);
		// xxx
		if (slot_max < slot_min)
			slot_max = slot_min;
	} else {
		slot_min = (size_t) atol(slots);
		slot_max = slot_min;
	}

	node->add_slot_range(slot_min, slot_max);
}

void redis_cluster::free_masters()
{
	std::map<string, redis_node*>::iterator it = masters_.begin();
	for (; it != masters_.end(); ++it) {
		it->second->clear_slaves(true);
		delete it->second;
	}
	masters_.clear();
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
