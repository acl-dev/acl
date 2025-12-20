#pragma once
#include "../acl_cpp_define.hpp"

#include "string.hpp"
#include "pipe_stream.hpp"

struct ACL_VSTRING;

namespace acl {

class ACL_CPP_API charset_conv : public pipe_stream {
public:
	charset_conv();
	~charset_conv();

	/**
	 * Set whether to allow directly copying invalid character sets
	 * @param onoff {bool} When true, during transcoding process, if illegal
	 * character set is encountered, directly copy, otherwise skip. By default,
	 * directly copy
	 */
	void set_add_invalid(bool onoff);

	/**
	 * Conversion function
	 * @param fromCharset {const char*} Source character set
	 * @param toCharset {const char*} Target character set
	 * @param in {const char*} Address of input source data (non-empty)
	 * @param n {size_t} Length of input source data (>0)
	 * @param out {string*} Store conversion result
	 * @return {bool} Whether conversion was successful
	 */
	bool convert(const char* fromCharset, const char* toCharset,
		const char* in, size_t n, string* out);

	/**
	 * If conversion fails, this function returns error reason
	 * @return {const char*} Error reason
	 */
	const char* serror() const;

	/**
	 * Reset transcoding state. This parser can be reused, but before using again,
	 * need to call
	 * set(from, to) to set source character set and target character set
	 */
	void reset();

	/* Streaming analysis process: update_begin->update->update ... ->update_finish */

	/**
	 * Initialize streaming analysis related parameters
	 * @param fromCharset {const char*} Source character set
	 * @param toCharset {const char*} Target character set
	 * @return {bool} Whether initialization was successful
	 */
	bool update_begin(const char* fromCharset, const char* toCharset);

	/**
	 * Perform character set conversion in streaming mode
	 * @param in {const char*} Source string
	 * @param len {size_t} in string length
	 * @param out {string*} Store conversion result
	 * @return {bool} Whether current conversion process was successful
	 */
	bool update(const char* in, size_t len, string* out);

	/**
	 * After streaming conversion ends, need to call this function to extract final
	 * conversion result
	 * @param out {string*} Store conversion result
	 */
	void update_finish(string* out);

	/**
	 * Create character set converter
	 * @param fromCharset {const char*} Source character set
	 * @param toCharset {const char*} Target character set
	 * @return {charset_conv*} If input parameters are illegal, or source character
	 * set
	 * and target character set are the same, or conversion between two character
	 * sets is not supported, returns NULL.
	 *  Need to call delete to delete after use
	 */
	static charset_conv* create(const char* fromCharset,
	                const char* toCharset);

	// pipe_stream virtual function override

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear();

public:

private:
	bool m_addInvalid;  // If invalid character set is encountered, whether to directly copy
	string  m_errmsg;
	string* m_pBuf;
	char  m_fromCharset[32];
	char  m_toCharset[32];
	void* m_iconv;
	ACL_VSTRING* m_pInBuf;
	ACL_VSTRING* m_pOutBuf;
	const char* m_pUtf8Pre;
};

/**
 * This C-like API can more conveniently and quickly perform character set
 * conversion
 * @param in {const char*} Input string (non-empty string)
 * @param len {size_t} Input string length (>0)
 * @param from_charset {const char*} Source character set encoding
 * @param to_charset {constchar*} Target character set encoding
 * @return {string} When returned string object is not empty (i.e.:
 * !string.empty()), it indicates conversion was successful
 */
ACL_CPP_API string strconv(const char* in, size_t len,
	const char* from_charset, const char* to_charset);

} // namespace acl

