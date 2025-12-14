#pragma once
#include "../acl_cpp_define.hpp"
#include <stdlib.h>
#include "stream.hpp"

namespace acl {

class string;

/**
 * Input stream base class. Users cannot accurately know whether the stream is
 * closed or has reached the end of the file. At the
 * end, users should call stream->eof() to determine.
 */

class ACL_CPP_API istream : virtual public stream {
public:
	istream() {}
	virtual ~istream() {}

	/**
	 * Read data from input stream.
	 * @param buf {void*} User buffer.
	 * @param size {size_t} User buffer size.
	 * @param loop {bool} Whether to read until size bytes are read.
	 * @return {int} Number of bytes read, -1 indicates closed or error, > 0
	 * indicates success.
	 */
	int read(void* buf, size_t size, bool loop = true);

	/**
	 * Read data from input stream until a specified tag string is encountered,
	 * then stop reading.
	 * @param buf {void*} User buffer.
	 * @param inout {size_t*} When not empty, *inout indicates available buf
	 *  space size. After return, stores data length in buf.
	 * @param tag {const char*} Tag string to search for.
	 * @param len {size_t} Length of tag string.
	 * @return {bool} Whether the specified tag string was encountered.
	 */

	bool readtags(void *buf, size_t* inout, const char *tag, size_t len);

	/**
	 * Read one line from input stream.
	 * @param buf {void*} User buffer.
	 * @param size_inout {size_t*} When not empty, *size_inout indicates available
	 * buf
	 *  space size. After return, stores data length in buf.
	 * @param nonl {bool} When set to true, will remove "\r\n"
	 * or "\n" at the end of the read line. *size_inout stores data length after
	 * removing "\r\n" or "\n"
	 * length. Otherwise, this function will read all "\r\n" or "\n" in the stream,
	 * and *size_inout
	 *  will store data length including "\r\n" or "\n".
	 * @return {bool} Whether one line was read. Returns false on error. For file
	 * stream
	 * characteristics, when reading the last line, if external data does not end
	 * with "\r\n" or "\n"
	 * it will also return false. At this time, users need to check if *size_inout
	 * value is greater than 0
	 *  to determine whether the last line was read.
	 */
	bool gets(void* buf, size_t* size_inout, bool nonl = true);

	/**
	 * Read a 64-bit integer from input stream.
	 * @param n {acl_int64&} 64-bit integer.
	 * @param loop {bool} Whether to read in loop mode until 8 bytes are read.
	 * @return {bool} Whether reading was successful.
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool read(__int64& n, bool loop = true);
#else
	bool read(long long int& n, bool loop = true);
#endif

	/**
	 * Read a 32-bit integer from input stream.
	 * @param n {int&} 32-bit integer.
	 * @param loop {bool} Whether to read in loop mode until 4 bytes are read.
	 * @return {bool} Whether reading was successful.
	 */
	bool read(int& n, bool loop = true);

	/**
	 * Read a 16-bit integer from input stream.
	 * @param n {short&} 16-bit integer.
	 * @param loop {bool} Whether to read in loop mode until 2 bytes are read.
	 * @return {bool} Whether reading was successful.
	 */
	bool read(short& n, bool loop = true);

	/**
	 * Read one byte from input stream.
	 * @param ch {char&}
	 * @return {bool} Whether reading was successful.
	 */
	bool read(char& ch);

	/**
	 * Read string data from input stream.
	 * @param s {string*} String buffer. Internally automatically expands buffer.
	 * @param loop {bool} Whether to read in loop mode until buffer is full. Buffer
	 *  capacity is s.capacity().
	 * @return {bool} Whether reading was successful.
	 */
	bool read(string& s, bool loop = true);
	bool read(string* s, bool loop = true);

	/**
	 * Read string data from input stream.
	 * @param s {string*} String buffer. Internally automatically expands buffer.
	 * @param max {size_t} Maximum length of data to be read.
	 * @param loop {bool} Whether to read until max bytes are read.
	 * @return {bool} Whether reading was successful.
	 */
	bool read(string& s, size_t max, bool loop = true);
	bool read(string* s, size_t max, bool loop = true);

	/**
	 * Read one line string from input stream.
	 * @param s {string&} String buffer. Internally automatically expands buffer.
	 * @param nonl {bool} Whether to automatically remove "\r\n" or "\n" at the end
	 * of the read line.
	 * @param max {size_t} When value > 0, this value limits the maximum length of
	 * the read line. When
	 * the received line length exceeds this value, this function will not return
	 * incomplete data, and internally will record an error;
	 *  When value = 0, there is no limit on data length.
	 * @return {bool} Whether one line was read.
	 * 1. When returning true, it means one line of data was read. If the read line
	 * only contains
	 *     "\r\n" or "\n", s content will be empty, i.e., s.empty() == true
	 * 2. When returning false, it means stream is closed or one line was not read.
	 * At this time s may
	 *     store incomplete data. Users need to check if (s.empty() == true).
	 */
	bool gets(string& s, bool nonl = true, size_t max = 0);
	bool gets(string* s, bool nonl = true, size_t max = 0);

	/**
	 * Read data from input stream until a specified tag string is encountered,
	 * then stop reading. The tag
	 * is used as a delimiter for data. The last part of the read data should be
	 * the tag string.
	 * @param s {string&} String buffer. Internally automatically expands buffer.
	 * @param tag {const string&} Tag string to search for.
	 * @return {bool} Whether the specified tag string was encountered.
	 */
	bool readtags(string& s, const string& tag);
	bool readtags(string* s, const string& tag);

	/**
	 * Read one byte character from input stream.
	 * @return {int} ASCII value of the read byte. Return value of -1 indicates
	 * peer closed or
	 *  error.
	 */
	int getch();

	/**
	 * Push back one byte character to input stream.
	 * @param ch {int} ASCII value of a character.
	 * @return {int} Return value. If same as ch value, it means correct. Otherwise
	 * indicates error.
	 */
	int ugetch(int ch);

	/**
	 * Detect whether current stream is readable (readable, connection closed or
	 * error indicates readable). This method is used in conjunction
	 * with xxx_peek() methods.
	 * @return {bool}
	 */
	bool readable() const;

	/**
	 * Determine whether current stream has readable data.
	 * @param timeo {int} Wait timeout value (seconds).
	 * @return {bool}
	 */
	bool read_wait(int timeo) const;

	/**
	 * Peek and read one line from source stream buffer.
	 * @param buf {string&} Buffer.
	 * @param nonl {bool} Whether to remove "\r\n" or "\n" from the read line.
	 * @param clear {bool} Whether to internally automatically clear buf buffer.
	 * @param max {int} When value > 0, limits the maximum length of the read line
	 * to prevent this
	 *  function from reading too much data.
	 * @return {bool} Whether one line was read; returning false does not indicate
	 * error.
	 * It only means one line was not read or incomplete data. Applications should
	 * call stream->eof()
	 * to determine whether the stream is closed. Additionally, if this function
	 * reads incomplete data, buf will store
	 *  this incomplete data.
	 * Note: To prevent buf from growing too large, callers should extract data
	 * from buf after using the data, even if only
	 *  one line is used, then call buf->clear() to prevent buf
	 *  memory from continuously growing.
	 */
	bool gets_peek(string& buf, bool nonl = true,
		bool clear = false, int max = 0);
	bool gets_peek(string* buf, bool nonl = true,
		bool clear = false, int max = 0);

	/**
	 * Peek and read data from source stream buffer.
	 * @param buf {string&} Buffer.
	 * @param clear {bool} Whether to internally automatically clear buf buffer at
	 * the beginning.
	 * @return {bool} Whether data was read. Returning false does not indicate
	 * error, it only means no data of required
	 * length was read. Applications should call stream->eof() to determine whether
	 * the stream is closed.
	 * Note: To prevent buf from growing too large, callers should extract data
	 * from buf after using the data, even if only
	 *  one line is used, then call buf->clear() to prevent buf
	 *  memory from continuously growing.
	 */
	bool read_peek(string& buf, bool clear = false);
	bool read_peek(string* buf, bool clear = false);

	/**
	 * Peek and read data from source stream buffer.
	 * @param buf {void*} Buffer.
	 * @param size {size_t} Buffer size.
	 * @return {int} Returns -1 to indicate stream closed or error, > 0 indicates
	 * number of bytes read,
	 * returning 0 indicates no readable data yet. You can continue reading. When
	 * return value < 0,
	 *  you should use eof() to determine whether the stream should be closed.
	 */
	int read_peek(void* buf, size_t size);

	/**
	 * Peek and read data of specified length from source stream buffer.
	 * @param buf {string&} Buffer.
	 * @param cnt {size_t} Length of data to be read.
	 * @param clear {bool} Whether to internally automatically clear buf buffer at
	 * the beginning.
	 * @return {bool} Whether data of required length was read. Returning false
	 * does not
	 * indicate no data of required length was read. Applications should call
	 * stream->eof() to determine
	 *  whether the stream is closed.
	 * Note: To prevent buf from growing too large, callers should extract data
	 * from buf after using the data, even if only
	 *  one line is used, then call buf->clear() to prevent buf
	 *  memory from continuously growing.
	 */
	bool readn_peek(string& buf, size_t cnt, bool clear = false);
	bool readn_peek(string* buf, size_t cnt, bool clear = false);

	/* The following are stream input operators. These are non-blocking input operators. Users need to
	 * call stream->eof() to determine whether the stream is closed or has reached the end of the file.
	 */

	istream& operator>>(string& s);
#if defined(_WIN32) || defined(_WIN64)
	istream& operator>>(__int64& n);
#else
	istream& operator>>(long long int& n);
#endif
	istream& operator>>(int& n);
	istream& operator>>(short& n);
	istream& operator>>(char& ch);

public:
	/**
	 * When process initializes, you can call this method to set the process-level
	 * read buffer size. Internal default value is 4096.
	 * @param n {size_t}
	 */
	static void set_rbuf_size(size_t n);
};

} // namespace acl

