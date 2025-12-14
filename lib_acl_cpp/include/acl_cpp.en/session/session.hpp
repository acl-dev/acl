#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include "../stdlib/dbuf_pool.hpp"
#include "../stdlib/string.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

// Buffer object definition used to store attribute values. This structure type is mainly added
// to be compatible with the case where attribute values can be binary.
typedef enum
{
	TODO_NUL,
	TODO_SET,
	TODO_DEL
} todo_t;

class ACL_CPP_API session_string : public string
{
public:
	session_string(size_t n = 64) : string(n), todo_(TODO_NUL) {}
	session_string(const session_string& ss) : string(ss)
	{
		todo_ = ss.todo_;
	}
	session_string(const string& s) : string(s), todo_(TODO_NUL) {}
	session_string(const char* s) : string(s), todo_(TODO_NUL) {}
	~session_string() {}

	session_string& operator= (const session_string& ss)
	{
		if (!ss.empty()) {
			this->copy(ss.c_str(), ss.size());
		}
		todo_ = ss.todo_;
		return *this;
	}

	todo_t todo_;
};

/**
 * session class, this class uses memcached to store session data.
 */
class ACL_CPP_API session : public dbuf_obj
{
public:
	/**
	 * When the constructor parameter sid is not empty, this session object uses that
	 * sid; otherwise, a sid will be automatically generated internally. The user should obtain
	 * this automatically generated sid through get_sid() in order to query the data corresponding
	 * to this sid each time.
	 * @param ttl {time_t} Specify the survival period of the session (seconds).
	 * @param sid {const char*} When not empty, the session's sid uses
	 *  this value. Otherwise, a random session sid will be generated internally. This random
	 *  sid can be obtained by calling get_sid(); of course, during use, the user
	 *  can also modify this object's session sid through set_sid();
	 *  Additionally, if this sid is empty, if the user wants to look up data corresponding to a certain sid,
	 *  the user must first call set_sid().
	 */
	session(time_t ttl = 0, const char* sid = NULL);
	virtual ~session(void);
	
	/**
	 * Reset internal state, clean up some temporary data.
	 */
	void reset(void);

	/**
	 * Get the unique ID identifier of this session object.
	 * @return {const char*} Non-empty.
	 */
	virtual const char* get_sid(void) const
	{
		return sid_.c_str();
	}

	/**
	 * Set the unique ID identifier of this session object.
	 * @param sid {const char*} Non-empty.
	 * Note: After calling this function, the previous intermediate cache data will be automatically cleared.
	 */
	void set_sid(const char* sid);

	/**
	 * When calling session class's set/set_ttl, if the last parameter delay is true,
	 * the data must be truly updated by calling this function.
	 * @return {bool} Whether the data update was successful.
	 */
	virtual bool flush();

	/**
	 * Add a new string attribute to the session, and set the
	 * session's expiration time interval (seconds) at the same time.
	 * @param name {const char*} Session name, non-empty.
	 * @param value {const char*} Session value, non-empty.
	 * @return {bool} Returns false to indicate an error.
	 */
	virtual bool set(const char* name, const char* value);

	/**
	 * Add a new attribute object to the session and set the session's expiration time interval (seconds).
	 * @param name {const char*} Session attribute name, non-empty.
	 * @param value {const char*} Session attribute value, non-empty.
	 * @param len {size_t} Value length of value.
	 * @return {bool} Returns false to indicate an error.
	 */
	virtual bool set(const char* name, const void* value, size_t len);

	/**
	 * Delayed addition of a new attribute object to the session and set the session's expiration time interval (seconds).
	 * Data update will be performed after the user calls session::flush, which can improve transmission efficiency.
	 * @param name {const char*} Session attribute name, non-empty.
	 * @param value {const char*} Session attribute value, non-empty.
	 * @param len {size_t} Value length of value.
	 * @return {bool} Returns false to indicate an error.
	 */
	virtual bool set_delay(const char* name, const void* value, size_t len);
	
	/**
	 * Get string type attribute value from the session.
	 * @param name {const char*} Session attribute name, non-empty.
	 * @return {const char*} Session attribute value. The returned pointer address is always non-empty. The user
	 *  can determine whether there is an error or it doesn't exist by checking if the return value is an empty string (i.e.: "\0").
	 *  Note: After this function returns non-empty data, the user should immediately save this return value, because the next
	 *      other function call may clear this temporary return data.
	 */
	const char* get(const char* name);

	/**
	 * Get binary data type attribute value from the session.
	 * @param name {const char*} Session attribute name, non-empty.
	 * @return {const session_string*} Session attribute value. Returns empty when
	 *  it indicates an error or doesn't exist.
	 *  Note: After this function returns non-empty data, the user should immediately save this return value, because the next
	 *      other function call may clear this temporary return data.
	 */
	virtual const session_string* get_buf(const char* name);

	/**
	 * Delete the specified attribute value from the session. When all variables are deleted,
	 * the entire object will be deleted from memcached.
	 * @param name {const char*} Session attribute name, non-empty.
	 * @return {bool} true means success (including non-existence), false means deletion failed.
	 *  Note: When using delayed mode to delete an attribute, the update instruction is delayed to be sent to the backend
	 *  cache server. Data update will be performed after the user calls session::flush, which
	 *  can improve transmission efficiency; otherwise, the data will be updated immediately.
	 */
	virtual bool del_delay(const char* name);
	virtual bool del(const char* name);

	/**
	 * Reset the session's cache time on the cache server.
	 * @param ttl {time_t} Survival period (seconds).
	 * @param delay {bool} When true, the update instruction is delayed to be sent to the backend
	 *  cache server. Data update will be performed after the user calls session::flush, which
	 *  can improve transmission efficiency. When false, the data will be updated immediately.
	 * @return {bool} Whether the setting was successful.
	 */
	bool set_ttl(time_t ttl, bool delay);

	/**
	 * Get the session survival period recorded in this session object. This value may
	 * be inconsistent with the time actually stored on the cache server, because other instances
	 * may have reset the session's survival period on the cache server.
	 * @return {time_t}
	 */
	time_t get_ttl(void) const
	{
		return ttl_;
	}

	/**
	 * Make the session be deleted from the server's cache, making the session invalid.
	 * @return {bool} Whether the session was invalidated.
	 */
	virtual bool remove(void) = 0;

	/**
	 * Get the attribute object collection corresponding to sid from the backend cache.
	 * @param attrs {std::map<string, session_string>&}
	 * @return {bool}
	 */
	virtual bool get_attrs(std::map<string, session_string>& attrs) = 0;

	/**
	 * Get the specified attribute collection corresponding to sid from the backend cache.
	 * @param names {const std::vector<string>&} Attribute name collection.
	 * @param values {std::vector<session_string>&} Store the corresponding attribute value result set.
	 * @return {bool} Whether the operation was successful.
	 */
	virtual bool get_attrs(const std::vector<string>& names,
		std::vector<session_string>& values);

	/**
	 * Write the attribute object collection corresponding to sid to the backend cache.
	 * @param attrs {std::map<string, session_string>&}
	 * @return {bool}
	 */
	virtual bool set_attrs(const std::map<string, session_string>& attrs) = 0;

protected:
	// Set the expiration time of data corresponding to sid.
	virtual bool set_timeout(time_t ttl) = 0;

protected:
	// Serialize session data.
	static void serialize(const std::map<string, session_string>& attrs,
		string& out);

	// Deserialize session data.
	static void deserialize(string& buf,
		std::map<string, session_string>& attrs);

	// Clear session attribute collection.
	static void attrs_clear(std::map<string, session_string>& attrs);

protected:
	session_string sid_;
	time_t ttl_;

	// This variable is mainly used in the set_ttl function. If it is inferred that sid_ is only newly generated
	// and has not been stored on the backend cache server, then set_ttl will not immediately update the backend
	// cache server.
	bool sid_saved_;
	bool dirty_;
	std::map<string, session_string> attrs_;
	std::map<string, session_string> attrs_cache_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

