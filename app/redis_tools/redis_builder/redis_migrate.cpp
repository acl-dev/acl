#include "stdafx.h"
#include "redis_util.h"
#include "redis_migrate.h"

redis_migrate::redis_migrate(std::vector<acl::redis_node*>& masters,
	const char* passwd)
	: masters_(masters)
{
	if (passwd && *passwd)
		passwd_ = passwd;
}

redis_migrate::~redis_migrate(void)
{
}

int redis_migrate::move_slots(std::vector<acl::redis_node*>& from,
	acl::redis_node& to, int count)
{
	assert(!from.empty());

	int base = count / (int) from.size(), nslots_moved = 0;
	int first = base + count % (int) from.size(), ret;

	std::vector<acl::redis_node*>::iterator it;
	for (it = from.begin(); it != from.end(); ++it) {
		// move some slots from one source node to the target node
		if (it == from.begin()) {
			ret = move_slots(**it, to, first);
		} else {
			ret = move_slots(**it, to, base);
		}
		if (ret < 0) {
			printf("move failed, stop!\r\n");
			break;
		}

		nslots_moved += ret;

		// check if all the slots have been moved
		if (nslots_moved >= count) {
			printf("moved %d slots ok\r\n", nslots_moved);
			break;
		}
	}

	printf("move over!\r\n");
	return nslots_moved;
}

bool redis_migrate::check_nodes_id(acl::redis& from, acl::redis& to)
{
	if (from_id_.empty() && !redis_util::get_node_id(from, from_id_)) {
		printf("can't get source node id, addr: %s\r\n",
			from.get_client_addr());
		return false;
	}
	if (to_id_.empty() && !redis_util::get_node_id(to, to_id_)) {
		printf("can't get target node id, addr: %s\r\n",
			to.get_client_addr());
		return false;
	}
	return true;
}

int redis_migrate::move_slots(acl::redis_node& from,
	acl::redis_node& to, int count)
{
	acl::redis_client from_conn(from.get_addr());
	from_conn.set_password(passwd_);
	acl::redis from_redis(&from_conn);

	acl::redis_client to_conn(to.get_addr());
	to_conn.set_password(passwd_);
	acl::redis to_redis(&to_conn);

	// get all the specified source node's slots
	const std::vector<std::pair<size_t, size_t> >& slots = from.get_slots();
	return move_slots(from_redis, to_redis, count, slots);
}

int redis_migrate::move_slots(acl::redis& from, acl::redis& to, int count,
	const std::vector<std::pair<size_t, size_t> >& slots)
{
	std::vector<std::pair<size_t, size_t> >::const_iterator cit;
	int nslots_moved = 0;

	// iterate the source node's slots, and move them to the target
	for (cit = slots.begin(); cit != slots.end(); ++cit) {
		size_t min = cit->first;
		size_t max = cit->second;
		int ret = move_slots(from, to, count, min, max);
		if (ret < 0) {
			return -1;
		}
		nslots_moved += ret;
	}
	
	return nslots_moved;
}

int redis_migrate::move_slots(acl::redis& from, acl::redis& to, int count,
	size_t min_slot, size_t max_slot)
{
	int nslots_moved = 0;

	for (size_t slot = min_slot; slot <= max_slot; slot++) {
		// move the specified slot from source to target
		if (!move_slot(slot, from, to)) {
			printf("move slots error, slot: %d\r\n",
				(int) slot);
			return -1;
		}
		nslots_moved++;

		// if the specified number slots have been moved ?
		if (nslots_moved >= count)
			return nslots_moved;
	}
	return nslots_moved;
}

bool redis_migrate::move_slot(size_t slot, acl::redis& from, acl::redis& to)
{
	if (!check_nodes_id(from, to)) {
		return false;
	}

	// set the slot in migrating status for the source node
	if (!from.cluster_setslot_migrating(slot, to_id_.c_str())) {
		return false;
	}
	// set the slot in importing status for the target node
	if (!to.cluster_setslot_importing(slot, from_id_.c_str())) {
		return false;
	}

	// the number of keys to be moved in each moving
	size_t max = 1000;
	std::list<acl::string> keys;

	std::list<acl::string>::const_iterator cit;

	while (true) {
		keys.clear();
		int nkeys = from.cluster_getkeysinslot(slot, max, keys);
		if (nkeys == 0) {
			break;
		}
		if (nkeys < 0) {
			printf("cluster_getkeysinslot error: %s\r\n",
				from.result_error());
			return false;
		}

		printf("Moving slot %d from %s to %s: ", (int) slot,
			from.get_client_addr(), to.get_client_addr());
		fflush(stdout);

		// move all the keys stored by the specifed key
		for (cit = keys.begin(); cit != keys.end(); ++cit) {
			if (!move_key((*cit).c_str(), from, to.get_client_addr())) {
				printf("move key: %s error, from: %s,"
					" to: %s\r\n", (*cit).c_str(),
					from.get_client_addr(),
					to.get_client_addr());
				return false;
			}

			putchar('.');
			fflush(stdout);
		}

		printf("\r\n");
	}

	return notify_cluster(slot, to_id_.c_str());
}

bool redis_migrate::move_key(const char* key, acl::redis& from,
	const char* to_addr)
{
	if (from.migrate(key, to_addr, 0, 15000)) {
		return true;
	}
	acl::string error(from.result_error());
	if (error.find("BUSYKEY", false) == NULL) {
		printf("move key: %s error: %s, from: %s, to: %s\r\n",
			key, error.c_str(), from.get_client_addr(), to_addr);
		return false;
	}

	printf("*** Target key: %s exists, Replace it fix? yes/no: ", key);
	char buf[256];
	int n = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
	if (n == ACL_VSTREAM_EOF) {
		printf("Input error, key: %s\r\n", key);
		return false;
	}

#define NQ(x, y) strcasecmp((x), (y))

	if (NQ(buf, "yes") && NQ(buf, "y") && NQ(buf, "true")) {
		printf("No replace key: %s in target: %s\r\n", key, to_addr);
		return false;
	}

	if (from.migrate(key, to_addr, 0, 15000, "REPLACE")) {
		return true;
	}

	printf("move key: %s error: %s, from: %s, to: %s\r\n",
		key, from.result_error(), from.get_client_addr(), to_addr);
	return false;
}

bool redis_migrate::notify_cluster(size_t slot, const char* id)
{
	acl::redis redis;
	std::vector<acl::redis_node*>::const_iterator cit;

	for (cit = masters_.begin(); cit != masters_.end(); ++cit) {
		acl::redis_client client((*cit)->get_addr());
		client.set_password(passwd_);
		redis.set_client(&client);
		redis.clear();

		if (!redis.cluster_setslot_node(slot, id)) {
			printf("cluster_setslot_node error: %s, slot: %d, "
				"addr: %s\r\n", redis.result_error(),
				(int) slot, (*cit)->get_addr());
			return false;
		}
	}

	printf("Notify all: slot %d, moved to %s ok\r\n", (int) slot, id);
	return true;
}
