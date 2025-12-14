#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include "master_conf.hpp"
#include <vector>

#ifndef ACL_CLIENT_ONLY

struct ACL_EVENT;

namespace acl {

class server_socket;
class event_timer;
class string;

ACL_CPP_API void master_log_enable(bool yes);
ACL_CPP_API bool master_log_enabled(void);

class ACL_CPP_API master_base : public noncopyable {
public:
	/**
	 * Set bool type configuration item
	 * @param table {master_bool_tbl*}
	 * @return {master_base&}
	 */
	master_base& set_cfg_bool(master_bool_tbl* table);

	/**
	 * Set int type configuration item
	 * @param table {master_int_tbl*}
	 * @return {master_base&}
	 */
	master_base& set_cfg_int(master_int_tbl* table);

	/**
	 * Set int64 type configuration item
	 * @param table {master_int64_tbl*}
	 * @return {master_base&}
	 */
	master_base& set_cfg_int64(master_int64_tbl* table);

	/**
	 * Set string type configuration item
	 * @param table {master_str_tbl*}
	 * @return {master_base&}
	 */
	master_base& set_cfg_str(master_str_tbl* table);

	/**
	 * Determine whether it is daemon mode controlled by acl_master
	 * @return {bool}
	 */
	bool daemon_mode() const;

	/////////////////////////////////////////////////////////////////////
	
	/**
	 * Set process-level timer. This function can only be set in main thread's running space (e.g., in function
	 * proc_on_init). When all timer tasks are executed, it will be automatically destroyed
	 * (i.e., internally will automatically call master_timer::destroy method)
	 * @param timer {event_timer*} Timer task
	 * @return {bool} Whether setting timer was successful
	 */
	bool proc_set_timer(event_timer* timer);

	/**
	 * Delete process-level timer
	 * @param timer {event_timer*} Timer task set by proc_set_timer
	 */
	void proc_del_timer(event_timer* timer);

protected:
	bool daemon_mode_;
	bool proc_inited_;
	std::vector<server_socket*> servers_;

	master_base();
	virtual ~master_base();

	/**
	 * When process starts, this function is called each time the service process successfully listens on a local address
	 * @param ss {const server_socket&} Listening object
	 */
	virtual void proc_on_listen(server_socket& ss) { (void) ss; }

	/**
	 * Callback function called before process switches user identity. Can perform some
	 * operations requiring root privileges in this function
	 */
	virtual void proc_pre_jail() {}

	/**
	 * Callback function called after process switches user identity. When this function is called, process
	 * permissions are at ordinary restricted level
	 */
	virtual void proc_on_init() {}

	/**
	 * This virtual method is called first before process exits. If subclass implementation returns false, it will sleep 1 second then
	 * continue calling this virtual method until subclass implementation returns true. This gives application layer a way to decide
	 * whether to exit immediately
	 *
	 */
	virtual bool proc_pre_exit() { return true; }

	/**
	 * Callback function called before process exits. After this function returns, process will officially exit. Note that this method
	 * will be called after the above proc_pre_exit call
	 */
	virtual void proc_on_exit() {}

	/**
	 * Virtual method callback when SIGHUP signal is received
	 */
	virtual bool proc_on_sighup(string&) { return true; }

	// Configuration object
	master_conf conf_;

protected:
	// Subclasses must call this method to set event engine handle
	void set_event(ACL_EVENT* event);

	/**
	 * Get event engine handle
	 * @return {ACL_EVENT*}
	 */
	ACL_EVENT* get_event() const {
		return event_;
	}

private:
	ACL_EVENT* event_;
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY

