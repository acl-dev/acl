#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

/**
 * Class definition related to Content-Type in HTTP headers, can parse the
 * following data:
 * Content-Type: application/x-www-form-urlencoded
 * Content-Type: multipart/form-data; boundary=xxx
 * Content-Type: application/octet-stream
 * Content-Type: text/html; charset=utf8
 * Content-Type: xxx/xxx; name=xxx
 * ...
 */
class ACL_CPP_API http_ctype : public noncopyable {
public:
	http_ctype();
	~http_ctype();

	/**
	 * Overloaded "=" operator for object copying
	 * @param ctype {const http_ctype&} Source object
	 * @return {http_ctype&}
	 */
	http_ctype& operator=(const http_ctype& ctype);

	/**
	 * Parse Content-Type field value in HTTP header
	 * @param cp {const char*} Content-Type field value, e.g.:
	 * application/x-www-form-urlencoded
	 * multipart/form-data; boundary=xxx
	 * application/octet-stream
	 * @return {bool} Whether input data is valid
	 */
	bool parse(const char* cp);

	/**
	 * Get text from Content-Type field value text/html; charset=utf8
	 * @return {const char*} Returns NULL if this data does not exist, generally
	 * because
	 *  parse failed
	 */
	const char* get_ctype() const;

	/**
	 * Get html from Content-Type field value text/html; charset=utf8
	 * @return {const char*} Returns NULL if this data does not exist
	 */
	const char* get_stype() const;

	/**
	 * Get boundary value xxx from Content-Type field value multipart/form-data;
	 * boundary=xxx
	 * @return {const char*} Returns NULL if this data does not exist
	 */
	const char* get_bound() const;

	/**
	 * Get name value name_xxx from Content-Type: xxx/xxx; name=name_xxx
	 * @return {const char*} Returns NULL if this data does not exist
	 */
	const char* get_name() const;

	/**
	 * Get utf8 from Content-Type field value text/html; charset=utf8
	 * @return {const char*} Returns NULL if this data does not exist
	 */
	const char* get_charset() const;

private:
	char* ctype_;
	char* stype_;
	char* name_;
	char* charset_;
	string* bound_;

	void reset();
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

