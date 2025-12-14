#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class ACL_CPP_API redis_slot {
public:
	/**
	 * Constructor
	 * constructor
	 * @param slot_min {size_t} Minimum hash slot value
	 *  the min hash-slot
	 * @param slot_max {size_t} Maximum hash slot value
	 *  the max hash-slot
	 * @param ip {const char*} IP address of the current redis-server
	 *  the given redis-server's ip
	 * @param port {int} Listening port of the current redis-server
	 *  the listening port of the given redis-server
	 */
	redis_slot(size_t slot_min, size_t slot_max, const char* ip, int port);
	redis_slot(const redis_slot& node);

	~redis_slot();

	/**
	 * Add a redis hash slot slave node to the current node
	 * add a slave slot node to the current node
	 * @param node {redis_slot*} A slave node storing hash slots
	 *  the slave slot node
	 */
	redis_slot& add_slave(redis_slot* node);

	/**
	 * Get all slave nodes of the current hash slot node
	 * get the slave nodes of the current node
	 * @return {const std::vector<redis_slot*>&}
	 */
	const std::vector<redis_slot*>& get_slaves() const {
		return slaves_;
	}

	/**
	 * Get the IP address of the current node
	 * get the ip of the current node
	 * @return {const char*}
	 */
	const char* get_ip() const {
		return ip_;
	}

	/**
	 * Get the port number of the current node
	 * get the port of the current node
	 * @return {int}
	 */
	int get_port() const {
		return port_;
	}

	/**
	 * Get the minimum value of the current hash slot node
	 * get the min hash slot of the current node
	 * @return {size_t}
	 */
	size_t get_slot_min() const {
		return slot_min_;
	}

	/**
	 * Get the maximum value of the current hash slot node
	 * get the max hash slot of the current node
	 * @return {size_t}
	 */
	size_t get_slot_max() const {
		return slot_max_;
	}

private:
	size_t slot_min_;
	size_t slot_max_;
	char ip_[128];
	int port_;

	std::vector<redis_slot*> slaves_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

