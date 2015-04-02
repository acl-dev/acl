#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_cluster.hpp"


namespace acl
{

#define INT_LEN		11
#define LONG_LEN	21

redis_cluster::redis_cluster()
: redis_command(NULL)
{
}

redis_cluster::redis_cluster(redis_client* conn)
: redis_command(conn)
{
}

redis_cluster::redis_cluster(redis_client_cluster* cluster, size_t max_conns)
: redis_command(cluster, max_conns)
{
}

redis_cluster::~redis_cluster()
{
	free_nodes();
}

bool redis_cluster::addslots(int first, ...)
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
	return addslots(slot_list);
}

bool redis_cluster::addslots(const int slot_list[], size_t n)
{
	build("CLUSTER", "ADDSLOTS", slot_list, n);
	return check_status();
}

bool redis_cluster::addslots(const std::vector<int>& slot_list)
{
	build("CLUSTER", "ADDSLOTS", slot_list);
	return check_status();
}

bool redis_cluster::delslots(int first, ...)
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
	return delslots(slot_list);
}

bool redis_cluster::delslots(const int slot_list[], size_t n)
{
	build("CLUSTER", "DELSLOTS", slot_list, n);
	return check_status();
}

bool redis_cluster::delslots(const std::vector<int>& slot_list)
{
	build("CLUSTER", "DELSLOTS", slot_list);
	return check_status();
}

int redis_cluster::getkeysinslot(size_t slot, size_t max,
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

bool redis_cluster::meet(const char* ip, int port)
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

bool redis_cluster::reset()
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

bool redis_cluster::reset_hard()
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

bool redis_cluster::reset_soft()
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

bool redis_cluster::setslot_importing(size_t slot, const char* src_node)
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

bool redis_cluster::setslot_migrating(size_t slot, const char* dst_node)
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

bool redis_cluster::setslot_stable(size_t slot)
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

bool redis_cluster::setslot_node(size_t slot, const char* node)
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

int redis_cluster::count_failure_reports(const char* node)
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

bool redis_cluster::failover()
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

bool redis_cluster::failover_force()
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

bool redis_cluster::failover_takeover()
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

bool redis_cluster::info(string& result)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "INFO";
	lens[1] = sizeof("INFO") - 1;

	build_request(2, argv, lens);
	return get_string(result) > 0 ? true : false;
}

bool redis_cluster::nodes(string& result)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "NODES";
	lens[1] = sizeof("NODES") - 1;

	build_request(2, argv, lens);
	return get_string(result) > 0 ? true : false;
}

bool redis_cluster::saveconfig()
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

bool redis_cluster::slaves(const char* node, std::vector<string>& result)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "SLAVES";
	lens[1] = sizeof("SLAVES") - 1;

	argv[2] = node;
	lens[2] = strlen(node);

	build_request(3, argv, lens);
	return get_strings(result) >= 0 ? true : false;
}

int redis_cluster::countkeysinslot(size_t slot)
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

bool redis_cluster::forget(const char* node)
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

int redis_cluster::keyslot(const char* key)
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

bool redis_cluster::replicate(const char* node)
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

bool redis_cluster::set_config_epoch(const char* epoch)
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

const std::vector<redis_node*>* redis_cluster::slots()
{
	free_nodes();

	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLUSTER";
	lens[0] = sizeof("CLUSTER") - 1;

	argv[1] = "SLOTS";
	lens[1] = sizeof("SLOTS") - 1;

	build_request(2, argv, lens);
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return NULL;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL)
		return NULL;

	for (size_t i = 0; i < size; i++)
	{
		const redis_result* rr = children[i];
		if (rr == NULL || rr->get_type() != REDIS_RESULT_ARRAY)
			continue;
		redis_node* node = get_master_node(rr);
		if (node != NULL)
			nodes_.push_back(node);
	}

	return &nodes_;
}

redis_node* redis_cluster::get_master_node(const redis_result* rr)
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

	redis_node* master = create_node(children[2], slot_max, slot_min);
	if (master == NULL)
		return NULL;

	redis_node* slave;
	for (size_t i = 3; i < size; i++)
	{
		slave = create_node(children[i], slot_max, slot_min);
		if (slave != NULL)
			master->add_slave(slave);
	}

	return master;
}

redis_node* redis_cluster::create_node(const redis_result* rr,
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

	redis_node* node = NEW redis_node(slot_min, slot_max, ip, port);
	return node;
}

void redis_cluster::free_nodes()
{
	std::vector<redis_node*>::iterator it;
	for (it = nodes_.begin(); it != nodes_.end(); ++it)
		delete *it;
	nodes_.clear();
}

} // namespace acl
