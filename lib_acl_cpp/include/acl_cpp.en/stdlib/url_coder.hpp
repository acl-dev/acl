#pragma once
#include "../acl_cpp_define.hpp"
#include "dbuf_pool.hpp"
#include <vector>

namespace acl {

class string;

struct URL_NV {
	char* name;
	char* value;
};

class ACL_CPP_API url_coder : public dbuf_obj {
public:
	/**
	 * Constructor
	 * @param nocase {bool} When true, parameter names are case-insensitive
	 * @param dbuf {dbuf_guard*} Memory pool object
	 */
	url_coder(bool nocase = true, dbuf_guard* dbuf = NULL);

	/**
	 * Constructor, construct through class instance object
	 * @param coder {const url_coder&}
	 * @param dbuf {dbuf_guard*} Memory pool object
	 */
	url_coder(const url_coder& coder, dbuf_guard* dbuf = NULL);

	~url_coder();

	/**
	 * URL encode data stored in params_ array
	 * @param buf {string&} Store encoded result
	 * @param clean {bool} Whether to clear the passed buf buffer
	 */
	void encode(string& buf, bool clean = true) const;

	/**
	 * Get string object converted from array object after encoding
	 * @return {const string&}
	 */
	const string& to_string() const;

	/**
	 * Parse URL-encoded string
	 * @param str {const char*} String in URL-encoded form
	 */
	void decode(const char* str);
	
	/**
	 * When using URL encoding, call this function to add variables
	 * @param name {const char*} Variable name
	 * @param value Variable value
	 * @param override {bool} If variable with same name exists, whether to directly override
	 * @return Returns reference to url_coder object
	 */
	url_coder& set(const char* name, const char* value,
		bool override = true);
	url_coder& set(const char* name, int value, bool override = true);
	url_coder& set(const char* name, bool override, const char* fmt, ...)
		ACL_CPP_PRINTF(4, 5);
	url_coder& set(const char* name, const char* fmt, va_list ap,
		bool override = true);

	/**
	 * Get value of a variable name in params_ array after URL decoding
	 * @param name {const char*} Variable name
	 * @param found {bool*} When this pointer is not NULL, will store whether name exists, mainly used
	 *  when name's value is empty
	 * @return {const char*} Returns NULL indicates does not exist
	 */
	const char* get(const char* name, bool* found = NULL) const;

	/**
	 * Get value of a variable name in params_ array after URL decoding
	 * @param name {const char*} Variable name
	 * @return {const char*} Returns NULL indicates does not exist or name's value is empty
	 *  Note: If name's value is empty, cannot correctly determine whether name exists
	 */
	const char* operator[](const char* name) const;

	/**
	 * Copy of URL encoder object
	 * @param coder {const url_coder&} Source URL encoder object
	 * @return {const url_coder&}
	 */
	const url_coder& operator =(const url_coder& coder);

	/**
	 * Get parameter array object
	 * @return {std::vector<URL_NV*>&}
	 */
	const std::vector<URL_NV*>& get_params() const {
		return params_;
	}

	/**
	 * Delete a variable from params_ parameter array
	 * @param name {const char*} Variable name
	 * @return {bool} Returns true indicates delete was successful, otherwise indicates does not exist
	 */
	bool del(const char* name);

	/**
	 * Reset parser state, clear internal cache
	 */
	void reset();

private:
	bool nocase_;
	dbuf_guard* dbuf_;
	dbuf_guard* dbuf_internal_;
	std::vector<URL_NV*> params_;
	string*  buf_;

	void init_dbuf(dbuf_guard* dbuf);
};

} // namespace acl end

