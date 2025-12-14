#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/pipe_stream.hpp"
#include "stream.hpp"

struct iovec;

namespace acl {

class string;

/**
 * Output stream processing class. Callers who want to know exactly whether output stream has error or is closed,
 * should call stream->eof() to determine
 */

class ACL_CPP_API ostream : virtual public stream , public pipe_stream {
public:
	ostream() {}
	virtual ~ostream() {}

	/**
	 * Write data to output stream
	 * @param data {const void*} Data pointer address
	 * @param size {size_t} data data length (bytes)
	 * @param loop {bool} Whether to ensure all data is output before returning. If true,
	 *  this function will not return until all data is output or error occurs. Otherwise, only writes once then returns,
	 *  but does not guarantee all data is written
	 * @param buffed {bool} Whether to cache data to be written first
	 * @return {int} Actual amount of data written. Returns -1 indicates error
	 */
	int write(const void* data, size_t size, bool loop = true,
		bool buffed = false);

	/**
	 * When sending data in datagram mode, can call this method to send data packet to target address
	 * @param data {const void*} Data address
	 * @param size {size_t} Data length
	 * @param dest_addr {const char*} Target address, format: ip|port
	 * @param flags {int} Send flag bits. Please refer to flags description in system sendto() api
	 * @return {int} Returns -1 indicates send failed
	 */
	int sendto(const void* data, size_t size,
		const char* dest_addr, int flags = 0);

	/**
	 * When sending data in datagram mode, can call this method to send data packet to target address
	 * @param data {const void*} Data address
	 * @param size {size_t} Data length
	 * @param dest_addr {const sockaddr*} Target address, format: ip|port
	 * @param addrlen {int} dest_addr address length
	 * @param flags {int} Send flag bits. Please refer to flags description in system sendto() api
	 * @return {int} Returns -1 indicates send failed
	 */
	int sendto(const void* data, size_t size,
		const struct sockaddr* dest_addr, int addrlen, int flags = 0);

	ssize_t send(const void* buf, size_t len, int flags);

	/**
	 * If write buffering mode is used, need to call this function at the end to flush buffer
	 * @return {bool} Returns false indicates write failed, possibly connection closed
	 */
	bool fflush();

	/**
	 * Determine whether current connection is writable
	 * @param timeo {int} Writable timeout (milliseconds)
	 * @return {bool}
	 */
	bool write_wait(int timeo) const;

	/**
	 * Write data to output stream
	 * @param v {const struct iovec*}
	 * @param count {int} Number of elements in array v
	 * @param loop {bool} Whether to ensure all data is output before returning. If true,
	 *  this function will not return until all data is output or error occurs. Otherwise, only writes once then returns,
	 *  but does not guarantee all data is written
	 * @return {int} Actual amount of data written. Returns -1 indicates error
	 */
	int writev(const struct iovec *v, int count, bool loop = true);

	/**
	 * Write data in formatted way, similar to vfprintf, ensures all data is written
	 * @param fmt {const char*} Format string
	 * @param ap {va_list} Variable argument list
	 * @return {int} Actual length of data written. Returns -1 indicates error
	 */
	int vformat(const char* fmt, va_list ap);

	/**
	 * Write data in formatted way, similar to fprintf, ensures all data is written
	 * @param fmt {const char*} Variable argument format string
	 * @return {int} Actual length of data written. Returns -1 indicates error
	 */
	int format(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * Write a 64-bit integer
	 * @param n {acl_int64} 64-bit data
	 * @return {int} Length of data written. Returns -1 indicates error
	 */
#if defined(_WIN32) || defined(_WIN64)
	int write(__int64 n);
#else
	int write(long long int n);
#endif

	/**
	 * Write a 32-bit integer
	 * @param n {int} 32-bit integer
	 * @return {int} Length of data written. Returns -1 indicates error
	 */
	int write(int n);

	/**
	 * Write a 16-bit short integer
	 * @param n {int} 16-bit integer
	 * @return {int} Length of data written. Returns -1 indicates error
	 */
	int write(short n);

	/**
	 * Write a byte
	 * @param ch {char}
	 * @return {int} Length of data written. Returns -1 indicates error
	 */
	int write(char ch);

	/**
	 * Output data in buffer
	 * @param s {const string&}
	 * @param loop {bool} Whether to require all output to complete before returning
	 * @return {int} Length of output data. Returns -1 indicates error
	 */
	int write(const string& s, bool loop = true);

	/**
	 * Output a line of string data, adding "\r\n" after given string
	 * @param s {const char*} String pointer, must end with '\0'
	 * @return {int} Length of output data. Returns -1 indicates error
	 */
	int puts(const char* s);

	/**
	 * The following functions are output operator overload functions, and all are blocking output processes.
	 * If want to determine whether output stream has error or is closed, should call stream->eof()
	 * to determine
	 */

	ostream& operator<<(const string& s);
	ostream& operator<<(const char* s);
#if defined(_WIN32) || defined(_WIN64)
	ostream& operator<<(__int64 n);
#else
	ostream& operator<<(long long int n);
#endif
	ostream& operator<<(int n);
	ostream& operator<<(short n);
	ostream& operator<<(char ch);

	// pipe_stream several virtual functions
	// Because it is output stream, only implements one
	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max);

	virtual int pop_end(string* out, size_t max) {
		(void) out;
		(void) max;
		return (0);
	}

public:
	/**
	 * Can call this method during process initialization to set internal write buffer size. Internal default is 512
	 * @param n {size_t}
	 */
	static void set_wbuf_size(size_t n);
};

} // namespace acl

