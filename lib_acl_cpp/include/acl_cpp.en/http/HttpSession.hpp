#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/dbuf_pool.hpp"
#include "../session/session.hpp"
#include <map>

#ifndef ACL_CLIENT_ONLY

namespace acl {

class session;

/**
 * Server-side HttpSession class. Currently data storage of this class can only support storage on memcached
 */
class ACL_CPP_API HttpSession : public dbuf_obj {
public:
	/**
	 * Constructor
	 * @param session {session&} Cache object
	 */
	explicit HttpSession(session& session);
	virtual ~HttpSession();

	/**
	 * Get string attribute value of session stored on server side by client
	 * @param name {const char*} Session attribute name, non-empty
	 * @return {const char*} Session attribute value. Return address is always non-NULL pointer. Users
	 *  can determine whether it exists or error occurred by checking whether return address is empty string ("\0")
	 *  Note: After this function returns non-empty data, users should immediately save this return value, because next
	 *      other function calls may clear this temporary return data
	 */
	virtual const char* getAttribute(const char* name) const;

	/**
	 * Get binary attribute value of session stored on server side by client
	 * @param name {const char*} Session attribute name, non-empty
	 * @param size {size_t*} When this parameter is not empty and attribute value is not empty, this pointer address
	 *  stores size of returned attribute value
	 * @return {const void*} Session attribute value. When it is NULL pointer, it indicates does not exist
	 *  or internal query failed
	 *  Note: After this function returns non-empty data, users should immediately save this return value, because next
	 *      other function calls may clear this temporary return data
	 */
	virtual const void* getAttribute(const char* name, size_t* size) const;

	/**
	 * Get all session attribute objects corresponding to client from server side, reducing number of interactions with server
	 * @param attrs {std::map<string, session_string>&}
	 * @return {bool} Whether successful
	 */
	virtual bool getAttributes(std::map<string, session_string>& attrs) const;

	/**
	 * Get corresponding attribute collection of client from server side
	 * @param names {const std::vector<string>&} Attribute name collection
	 * @param values {std::vector<session_string>&} Store corresponding attribute value result set
	 * @return {bool} Whether successful
	 */
	virtual bool getAttributes(const std::vector<string>& names,
		std::vector<session_string>& values) const;

	/**
	 * Set string attribute value of session on server side
	 * @param name {const char*} Session attribute name, non-empty
	 * @param value {const char*} Session attribute value, non-empty
	 * @return {bool} Returns false indicates setting failed
	 */
	virtual bool setAttribute(const char* name, const char* value);

	/**
	 * Set binary attribute value of session on server side
	 * @param name {const char*} Session attribute name, non-empty
	 * @param value {const void*} Session attribute value, non-empty
	 * @param len {size_t} value data length
	 * @return {bool} Returns false indicates setting failed
	 */
	virtual bool setAttribute(const char* name, const void* value, size_t len);

	/**
	 * Set session attribute collection on server side, reducing number of interactions with backend
	 * @param attrs {const std::map<string, session_string>&} Attribute collection object
	 * @return {bool} Whether setting was successful
	 */
	virtual bool setAttributes(const std::map<string, session_string>& attrs);

	/**
	 * Delete an attribute value in client session
	 * @param name {const char*} Session attribute name, non-empty
	 * @return {bool} Whether deletion was successful
	 */
	virtual bool removeAttribute(const char* name);

	/**
	 * Set lifetime of session on cache server
	 * @param ttl {time_t} Lifetime (seconds)
	 * @return {bool} Whether successful
	 */
	virtual bool setMaxAge(time_t ttl);

	/**
	 * Delete session from server-side cache, i.e., invalidate session
	 * @return {bool} Whether session was invalidated
	 */
	virtual bool invalidate();

	/**
	 * Get generated session ID identifier
	 * @return {const char*} Always returns non-NULL pointer ending with '\0'. Can determine whether
	 *  sid exists based on whether return value is empty string ("\0")
	 */
	const char* getSid() const;

protected:
	session& session_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

