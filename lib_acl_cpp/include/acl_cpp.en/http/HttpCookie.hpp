#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../stdlib/dbuf_pool.hpp"
#include "http_type.hpp"

namespace acl {

/**
 * Cookie object class in HTTP protocol header
 */
class ACL_CPP_API HttpCookie : public dbuf_obj {
public:
	/**
	 * Constructor
	 * @param name {const char*} Cookie name, non-empty string with string length > 0
	 * @param value {const char*} Cookie value, pointer is non-empty, string length can be 0
	 * Note: If input two parameters do not meet conditions, internally will generate assertion
	 * @param dbuf {dbuf_guard*} When not empty, will be used as memory allocation pool
	 */
	HttpCookie(const char* name, const char* value, dbuf_guard* dbuf = NULL);

	/**
	 * When using this constructor, can use setCookie to add cookie items
	 * @param dbuf {dbuf_guard*} When not empty, will be used as memory allocation pool
	 */
	explicit HttpCookie(dbuf_guard* dbuf = NULL);

	/**
	 * Copy constructor
	 * @param cookie {const HttpCookie*} Non-NULL, internally will copy its member variables
	 * @param dbuf {dbuf_guard*} When not empty, will be used as memory allocation pool
	 */
	explicit HttpCookie(const HttpCookie* cookie, dbuf_guard* dbuf = NULL);

	/**
	 * Destructor
	 */
	~HttpCookie();

	/**
	 * Parse data like Set-Cookie: xxx=xxx; domain=xxx; expires=xxx; path=xxx; max-age=xxx; ...
	 * @param value {const char*} Content like xxx=xxx; domain=xxx; ...
	 * @return {bool} Whether input data is legal
	 */
	bool setCookie(const char* value);

	/**
	 * Dynamically created class objects are released through this function
	 */
	void destroy();

	/**
	 * Set cookie's scope
	 * @param domain {const char*} Cookie scope
	 * @return {HttpCookie&} Returns reference to this object for convenient chained operations
	 */
	HttpCookie& setDomain(const char* domain);

	/**
	 * Set cookie's path field
	 * @param path {const char*} path field value
	 * @return {HttpCookie&} Returns reference to this object for convenient chained operations
	 */
	HttpCookie& setPath(const char* path);

	/**
	 * Set cookie's expiration time period, i.e., current time plus input time is cookie's
	 * expiration time
	 * @param timeout {time_t} Expiration time value (unit: seconds). Current time plus this time
	 * is cookie's expiration time
	 * @return {HttpCookie&} Returns reference to this object for convenient chained operations
	 */
	HttpCookie& setExpires(time_t timeout);

	/**
	 * Set cookie's expiration timestamp string
	 * @param expires {const char*} Expiration timestamp
	 * @return {HttpCookie&} Returns reference to this object for convenient chained operations
	 */
	HttpCookie& setExpires(const char* expires);

	/**
	 * Set cookie's lifetime
	 * @param max_age {int} Lifetime in seconds
	 * @return {HttpCookie&} Returns reference to this object for convenient chained operations
	 */
	HttpCookie& setMaxAge(int max_age);

	/**
	 * Add other attribute values for this cookie object
	 * @param name {const char*} Attribute name
	 * @param value {const char*} Attribute value
	 * @return {HttpCookie&} Returns reference to this object for convenient chained operations
	 */
	HttpCookie& add(const char* name, const char* value);

	/**
	 * Get cookie name, depends on constructor input value
	 * @return {const char*} String with length > 0, always non-NULL pointer
	 * Note: User must call HttpCookie(const char*, const char*) constructor
	 *     or call setCookie(const char*) successfully before calling this function,
	 *     otherwise returned data is "\0"
	 */
	const char* getName() const;

	/**
	 * Get cookie value, depends on constructor input value
	 * @return {const char*} Non-NULL pointer, may be empty string ("\0")
	 */
	const char* getValue() const;

	/**
	 * Get expiration time in string format
	 * @return {const char*} Non-NULL pointer. Return value "\0" indicates does not exist
	 */
	const char* getExpires() const;

	/**
	 * Get cookie scope
	 * @return {const char*} Non-NULL pointer. Return value "\0" indicates does not exist
	 */
	const char* getDomain() const;

	/**
	 * Get cookie's storage path
	 * @return {const char*} Non-NULL pointer. Return value "\0" indicates does not exist
	 */
	const char* getPath() const;

	/**
	 * Get cookie's lifetime
	 * @return {int} Returns -1 indicates Max-Age field does not exist
	 */
	int  getMaxAge() const;

	/**
	 * Get parameter value corresponding to parameter name
	 * @param name {const char*} Parameter name
	 * @param case_insensitive {bool} Whether case-sensitive. true indicates
	 *  case-insensitive
	 * @return {const char*} Non-NULL pointer. Return value "\0" indicates does not exist
	 */
	const char* getParam(const char* name,
		bool case_insensitive = true) const;

	/**
	 * Get all attributes and attribute values of this cookie object except cookie name and cookie value
	 * @return {const std::list<HTTP_PARAM*>&}
	 */
	const std::list<HTTP_PARAM*>& getParams() const;

private:
	dbuf_guard* dbuf_internal_;
	dbuf_guard* dbuf_;
	char  dummy_[1];
	char* name_;
	char* value_;
	std::list<HTTP_PARAM*> params_;

	bool splitNameValue(char* data, HTTP_PARAM* param);

protected:
//	HttpCookie(HttpCookie&) {}
//	HttpCookie(const HttpCookie&) {}
};

} // namespace acl end

