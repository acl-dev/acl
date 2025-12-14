#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class string;
class mime_code;

struct rfc2047_entry {
	string* pData;		// Data content
	string* pCharset;	// Character set
	char  coding;		// Encoding format, B indicates BASE64, Q indicates QP
};

class ACL_CPP_API rfc2047 : public noncopyable {
public:
	/**
	 * Constructor
	 * @param strip_sp {bool} Whether to remove carriage return line feed and spaces and TABs at beginning of each line during decoding process
	 * @param addCrlf {bool} Whether to automatically add "\r\n" when data is relatively long during encoding process
	 */
	rfc2047(bool strip_sp = true, bool addCrlf = true);
	~rfc2047();

	/**
	 * Stream parsing data. Can call this function in a loop, adding partial data each time
	 * until adding is complete
	 * @param in {const char*} Input source string
	 * @param n {int} Length of in input string
	 */
	void decode_update(const char* in, int n);

	/**
	 * Convert rfc2047 parsing result to specified character set string. If cannot
	 * correctly convert, retains source string content
	 * @param to_charset {const char*} Target character set
	 * @param out {string*} Store conversion result
	 * @param addInvalid {bool} When true, during transcoding process, if illegal
	 *  character set is encountered, directly copy, otherwise skip. By default, directly copy
	 * @return {bool} Whether conversion was successful
	 */
	bool decode_finish(const char* to_charset, string* out,
		bool addInvalid = true);

	/**
	 * Add data during rfc2047 encoding process
	 * @param in {const char*} Input data
	 * @param n {int} in data length
	 * @param out {string*} Store encoding result
	 * @param charset {const char*} Character set type of input data
	 * @param coding {char} Encoding type. Supported encoding types:
	 *   B: base64, Q: quoted_printable
	 * @return {bool} Whether input parameters are correct and encoding was successful
	 */
	bool encode_update(const char* in, int n, string* out,
		const char* charset = "gb2312", char coding = 'B');

	/**
	 * Encode data added by encode_update and store in user-specified buffer
	 * @param  out {string*} User buffer for storing encoding result
	 * @return {bool} Whether successful
	 */
	bool encode_finish(string* out);

	/**
	 * Static encoder
	 * @param in {const char*} Input data address
	 * @param n {int} Data length
	 * @param out {string*} Buffer for storing encoding result
	 * @param charset {const char*} Character set of input data
	 * @param coding {char} Encoding type. Supported encoding types:
	 *   B: base64, Q: quoted_printable
	 * @param addCrlf {bool} Whether to automatically add "\r\n" when data is relatively long during encoding process
	 * @return {bool} Whether encoding was successful
	 */
	static bool encode(const char* in, int n, string* out,
		const char* charset = "gb2312", char coding = 'B',
		bool addCrlf = true);

	/**
	 * Static decoder
	 * @param in {const char*} Input data address
	 * @param n {int} Data length
	 * @param out {string*} Buffer for storing decoding result
	 * @param to_charset {const char*} Target character set
	 * @param strip_sp {bool} Whether to remove carriage return line feed and spaces and TABs at beginning of each line
	 * @param addInvalid {bool} When true, during transcoding process, if illegal
	 *  character set is encountered, directly copy, otherwise skip. By default, directly copy
	 * @return {bool} Whether decoding was successful
	 */
	static bool decode(const char* in, int n, string* out,
		const char* to_charset = "gb2312", bool strip_sp = false,
		bool addInvalid = true);

	/**
	 * Give parsing result in linked list form
	 * @return {const std::list<rfc2047_entry*>&}
	 */
	const std::list<rfc2047_entry*>& get_list() const;

	/**
	 * After resetting parser state, this parser can be used again
	 * @param strip_sp {bool} Whether to remove carriage return line feed and spaces and TABs at beginning of each line
	 */
	void reset(bool strip_sp = true);

	/**
	 * Debug output parsing result
	 */
	void debug_rfc2047() const;

private:
	std::list<rfc2047_entry*> m_List;
	rfc2047_entry* m_pCurrentEntry;
	mime_code* m_coder;
	int   m_status;
	bool  m_stripSp;
	bool  m_addCrlf;
	char  m_lastCh;

public:
	// The following functions are only for internal use

	int status_next(const char* s, int n);
	int status_data(const char* s, int n);
	int status_charset(const char* s, int n);
	int status_coding(const char* s, int n);
	int status_equal_question(const char* s, int n);
	int status_question_first(const char* s, int n);
	int status_question_second(const char* s, int n);
	int status_question_equal(const char* s, int n);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

