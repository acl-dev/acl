#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/pipe_stream.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {
	
class string;

class ACL_CPP_API mime_code : public pipe_stream {
public:
	/**
	 * Constructor
	 * @param addCrlf {bool} Whether to add "\r\n" at the end when not in streaming encoding mode
	 * @param addInvalid {bool} Whether to copy illegal characters as-is when in streaming decoding mode
	 * @param encoding_type {const char*} Encoding type identifier
	 */
	mime_code(bool addCrlf, bool addInvalid, const char* encoding_type);
	virtual ~mime_code() = 0;

	/**
	 * Get encoding type identifier
	 * @return {const char*}
	 */
	const char* get_encoding_type() const {
		return encoding_type_;
	}

	/* Streaming encoding function, usage: encode_update->encode_update->...->encode_finish */

	/**
	 * Encoding process, add source data, result stored in out. If input
	 * data does not meet encoding cache condition, internally only temporarily caches without encoding
	 * @param src {const char*} Source data address
	 * @param n {int} Source data length
	 * @param out {string*} Store encoding result. Can determine whether this function performed
	 *  encoding process or temporary caching process by comparing out->length() before and after calling this function.
	 *  If result data in out is used, should call out->clear() after use to clear used data
	 */
	virtual void encode_update(const char *src, int n, string* out);

	/**
	 * After encoding ends, need to call this function to perform final encoding on source data
	 * that may exist in temporary cache
	 * @param out {string*} Store encoding result. Can determine whether this function performed
	 *  encoding process or temporary caching process by comparing out->length() before and after calling this function.
	 *  If result data in out is used, should call out->clear() after use to clear used data
	 */
	virtual void encode_finish(string* out);

	/* Streaming decoding function, usage: decode_update->decode_update->...->decode_finish */

	/**
	 * Decoding process, add encoded data, decode through this function. If
	 * input data does not meet byte count condition for decoding, internally only temporarily caches
	 * this data, performs decoding only when decoding condition is met
	 * @param src {const char*} Encoded data
	 * @param n {int} Data length
	 * @param out {string*} Store decoding result. Can determine whether this function performed
	 *  decoding process or temporary caching process by comparing out->length() before and after calling this function.
	 *  If result data in out is used, should call out->clear() after use to clear used data
	 */
	virtual void decode_update(const char *src, int n, string* out);

	/**
	 * After decoding ends, need to call this function to perform final decoding on source data
	 * that may exist in temporary cache
	 * @param out {string*} Store decoding result. Can determine whether this function performed
	 *  decoding process or temporary caching process by comparing out->length() before and after calling this function.
	 *  If result data in out is used, should call out->clear() after use to clear used data
	 */
	virtual void decode_finish(string* out);

	/**
	 * Reset internal buffer
	 */
	virtual void reset();

	/**
	 * Set whether to automatically add "\r\n" to each encoding segment during encoding process
	 * @param on {bool}
	 */
	virtual void add_crlf(bool on);

	/**
	 * During decoding process, if illegal character is encountered, whether to add it to decoding result
	 * @param on {bool}
	 */
	virtual void add_invalid(bool on);

	/**
	 * Generate corresponding decoding table based on input encoding table
	 * @param toTab {const unsigned char*} Encoding table string
	 * @param out {string*} Store result
	 */
	static void create_decode_tab(const unsigned char *toTab, string *out);

	/**
	 * If subclass does not override above virtual functions and therefore uses base class's above default virtual functions,
	 * subclass must call this function to set its own encoding table, decoding table and padding character
	 * @param toTab {const unsigned char*} Encoding table
	 * @param unTab {const unsigned char*} Decoding table
	 * @param fillChar {unsigned char} Padding character
	 */
	void init(const unsigned char* toTab,
		const unsigned char* unTab, unsigned char fillChar);

	/**
	 * Set working state of transcoder. Because this transcoder consists of encoder and decoder,
	 * when working in pipe_stream mode, must specify state of this transcoder
	 * to specify whether it is in encoder state or decoder state
	 * @param encoding {bool} If true, indicates encoder state, otherwise
	 *  decoder state
	 */
	void set_status(bool encoding = true);

	// pipe_stream virtual function override

	virtual int push_pop(const char* in, size_t len, string* out, size_t max);
	virtual int pop_end(string* out, size_t max);
	virtual void clear();

	/**
	 * Static function, get corresponding encoder/decoder object based on encoding type MIME_ENC_XXX (see: mime_define.hpp).
	 * Only when encoding type is MIME_ENC_QP,
	 * MIME_ENC_BASE64, MIME_ENC_UUCODE, MIME_ENC_XXCODE
	 * @param encoding {int} Encoding type, can only be MIME_ENC_QP, MIME_ENC_BASE64,
	 *  MIME_ENC_UUCODE, MIME_ENC_XXCODE
	 * @param warn_unsupport {bool} Whether to record warning information when matching encoder object is not found
	 * @return {mime_code*} Encoder object. Returns NULL when matching encoding type is not found
	 */
	static mime_code* create(int encoding, bool warn_unsupport = true);

private:
	void encode(string* out);
	void decode(string* out);

	char  m_encodeBuf[57];
	int   m_encodeCnt;
	char  m_decodeBuf[76];
	int   m_decodeCnt;
	bool  m_addCrLf;
	bool  m_addInvalid;
	bool  m_encoding;
	const unsigned char *m_toTab;
	const unsigned char *m_unTab;
	unsigned char m_fillChar;
	string* m_pBuf;
	char* encoding_type_;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

