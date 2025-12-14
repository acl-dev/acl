#pragma once
#include "../stdlib/noncopyable.hpp"

namespace acl {

class tcp_manager;
class tcp_pool;
class string;

/**
 * This class wraps tcp_manager management class, can dynamically add target server addresses, and dynamically create connection pools
 * with each server
 */
class ACL_CPP_API tcp_ipc : public noncopyable {
public:
	tcp_ipc();
	~tcp_ipc();

	/**
	 * Set maximum connection limit for connection pool built with each server
	 * @param max {int} Maximum connection limit for each connection pool. When <= 0, there is no connection limit
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& set_limit(int max);

	/**
	 * Set idle time for each connection in connection pool. When connection idle time exceeds set value, it will be closed
	 * @param ttl {int} Maximum timeout for idle connections
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& set_idle(int ttl);

	/**
	 * Set network connection timeout for each connection
	 * @param conn_timeout {int} Network connection timeout (seconds)
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& set_conn_timeout(int conn_timeout);

	/**
	 * Set network read/write timeout for each connection
	 * @param timeout {int} Read/write timeout (seconds)
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& set_rw_timeout(int timeout);

	/**
	 * Get TCP manager object
	 * @return {tcp_manager&}
	 */
	tcp_manager& get_manager(void) const;

	/**
	 * Can call this method to explicitly add a server address. Only adds when address does not exist
	 * @param addr {const char*} Server address, format: IP:PORT
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& add_addr(const char* addr);

	/**
	 * Delete specified connection pool object based on server address. When connection pool object is being referenced, this object
	 * will not be deleted, but uses delayed deletion method. Only after the last connection is returned will this connection pool object
	 * be truly deleted
	 * @param addr {const char*} Server address, format: IP:PORT
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& del_addr(const char* addr);

	/**
	 * Check whether specified server address exists successfully
	 * @param addr {const char*} Server address, format: IP:PORT
	 * @return {bool}
	 */
	bool addr_exist(const char* addr);

	/**
	 * Get collection of all current server addresses
	 * @param addrs {std::vector<string>&} Store result set
	 */
	void get_addrs(std::vector<string>& addrs);

	/**
	 * Send data packet of specified length to server
	 * @param addr {const char*} Specified target server address
	 * @param data {const void*} Address of data packet to be sent
	 * @param len {unsigned int} Data length
	 * @param out {string*} When this object is not NULL, it indicates that response data
	 *  needs to be read from server. Response result will be stored in this buffer. If this object is NULL, it means no response data
	 *  needs to be read from server
	 * @return {bool} Whether sending was successful
	 */
	bool send(const char* addr, const void* data, unsigned int len,
		string* out = NULL);

	/**
	 * Send data packet to all servers
	 * @param data {const void*} Address of data packet to be sent
	 * @param len {unsigned int} Data length
	 * @param exclusive {bool} When sending broadcast packet, whether to add thread lock to prevent other threads
	 *  from competing for internal connection pool resources
	 * @param check_result {bool} Whether to read server response to prove server received data
	 * @param nerr {unsigned *} When not NULL, stores number of failed servers
	 * @return {size_t} Returns number of servers sent to
	 */
	size_t broadcast(const void* data, unsigned int len,
		bool exclusive = true, bool check_result = false,
		unsigned* nerr = NULL);

private:
	tcp_manager* manager_;
	int max_;
	int ttl_;
	int conn_timeout_;
	int rw_timeout_;

	bool send(tcp_pool&, const void*, unsigned int, string*);
};

} // namespace acl

