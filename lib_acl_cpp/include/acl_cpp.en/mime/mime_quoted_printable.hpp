#pragma once
#include "../acl_cpp_define.hpp"
#include "mime_code.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class string;

class ACL_CPP_API mime_quoted_printable : public mime_code {
public:
	/**
	 * Constructor
	 * @param addCrlf {bool} Whether to add "\r\n" at the end when not in streaming encoding mode
	 * @param addInvalid {bool} Whether to copy illegal characters as-is when in streaming decoding mode
	 */
	mime_quoted_printable(bool addCrlf = false, bool addInvalid = false);
	~mime_quoted_printable();

	// Base class virtual function override

	/* Streaming encoding function, usage: encode_update->encode_update->...->encode_finish */

	/**
	 * Streaming encoding function
	 * @param src {const char*} Original data
	 * @param n {int} src data length
	 * @param out {string*} Store encoding result. Can test whether out has result data through out->empty(),
	 *  get length of result data in out through out->length(). Note: After using result data in out, must call out->clear()
	 *  to clear used result data. Can also call this function multiple times, then get all data at once after finally calling
	 *  encode_finish
	 */
	void encode_update(const char *src, int n, string* out);

	/**
	 * Streaming encoding end function. After calling this function, extract final result data
	 * @param out {string*} Store encoding result. Can test whether out has result data through out->empty(),
	 *  get length of result data in out through out->length(). Note: After using result data in out, must call out->clear()
	 *  to clear used result data
	 */
	void encode_finish(string* out);

	/* Streaming decoding function, usage: decode_update->decode_update->...->decode_finish */

	/**
	 * Streaming decoding function
	 * @param src {const char*} Original data
	 * @param n {int} src data length
	 * @param out {string*} Store decoding result. Can test whether out has result data through out->empty(),
	 *  get length of result data in out through out->length(). Note: After using result data in out, must call out->clear()
	 *  to clear used result data. Can also call this function multiple times, then get all data at once after finally calling
	 *  decode_finish
	 */
	void decode_update(const char *src, int n, string* out);

	/**
	 * Streaming decoding end function. After calling this function, extract final result data
	 * @param out {string*} Store decoding result. Can test whether out has result data through out->empty(),
	 *  get length of result data in out through out->length(). Note: After using result data in out, must call out->clear()
	 *  to clear used result data
	 */
	void decode_finish(string* out);

	/**
	 * Static encoding function, directly encode input data and store in user buffer
	 * User buffer
	 * @param in {const char*} Input data address
	 * @param n {int} Input data length
	 * @param out {string*} Buffer for storing result
	 */
	static void encode(const char* in, int n, string* out);

	/**
	 * Static decoding function, directly parse input data and store in user buffer
	 * @param in {const char*} Input data address
	 * @param n {int} Data length
	 * @param out {string*} Store parsing result
	 */
	static void decode(const char* in, int n, string* out);

	/**
	 * Reset encoder/decoder state
	 */
	void reset();

	/**
	 * Set whether to add "\r\n" at end of encoding
	 * @param on {bool}
	 */
	void add_crlf(bool on);

	/**
	 * Set whether to copy illegal characters during decoding process
	 * @param on {bool}
	 */
	void add_invalid(bool on);

protected:
private:
	void encode(string* out);
	void decode(string* out);

	bool hex_decode(unsigned char first, unsigned char second,
		unsigned int *result);

	char  m_encodeBuf[72];
	int   m_encodeCnt;
	char  m_decodeBuf[144];
	int   m_decodeCnt;
	bool  m_addCrLf;
	bool  m_addInvalid;
};

}  // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

