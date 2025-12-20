#pragma once
#include "../acl_cpp_define.hpp"
#include "mime_code.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class ACL_CPP_API mime_base64 : public mime_code {
public:
	/**
	 * Constructor
	 * @param addCrlf {bool} Whether to add "\r\n" at the end when non-streaming
	 * encoding
	 * @param addInvalid {bool} Whether to copy invalid characters as-is when
	 * streaming decoding
	 */
	mime_base64(bool addCrlf = false, bool addInvalid = false);
	~mime_base64();

	/**
	 * Static encoding function, directly encode input data and store in user
	 * buffer
	 * User buffer
	 * @param in {const char*} Input data address
	 * @param n {int} Input data length
	 * @param out {string*} Buffer to store results
	 */
	static void encode(const char* in, int n, string* out);

	/**
	 * Static decoding function, directly parse input data and store in user buffer
	 * @param in {const char*} Input data address
	 * @param n {int} Data length
	 * @param out {string*} Buffer to store parsing results
	 */
	static void decode(const char* in, int n, string* out);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

