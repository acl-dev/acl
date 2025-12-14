#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

namespace acl {

class istream;

class ACL_CPP_API md5 : public noncopyable {
public:
	md5();
	~md5();

	/**
	 * Can call this function in a loop to add data to be md5
	 * @param dat {const void*} Data address
	 * @param len {size_t} dat data length
	 * @return {md5&}
	 */
	md5& update(const void* dat, size_t len);

	/**
	 * Must call this function to indicate md5 process ends
	 * @return {md5&}
	 */
	md5& finish();

	/**
	 * Reset md5 algorithm state, allowing reuse of the same md5 object
	 * @return {md5&}
	 */
	md5& reset();

	/**
	 * Get md5 result value in binary format
	 * @return {const char*} Return value is always non-empty, and buffer length is 16 bytes
	 */
	const char* get_digest() const;

	/**
	 * Get md5 result value represented as string
	 * @return {const char*} Return value is always non-empty, ends with \0, and string
	 *  length is 32 bytes
	 */
	const char* get_string() const;

	/**
	 * Calculate signature value of data using md5 algorithm, get 128-bit (i.e., 16-byte) binary result
	 * @param dat {const void*} Source data
	 * @param dlen {size_t} dat data length
	 * @param key {const char*} When not empty, used as key data
	 * @param klen {size_t} When key is not empty, indicates length of key
	 * @param out {void*} Store md5 result
	 * @param size {size_t} out size, should be at least 16 bytes
	 * @return {const char*} Returns address storing result (i.e., out address)
	 */
	static const char* md5_digest(const void *dat, size_t dlen,
		const void *key, size_t klen, void* out, size_t size);

	/**
	 * Calculate signature value of data using md5 algorithm, get result in string form
	 * @param dat {const void*} Source data
	 * @param dlen {size_t} dat data length
	 * @param key {const char*} When not empty, used as key data
	 * @param klen {size_t} When key is not empty, indicates length of key
	 * @param out {void*} Store md5 result
	 * @param size {size_t} out size, should be at least 33 bytes
	 * @return {const char*} Returns address storing result (i.e., out address),
	 *  and return value is a 32-byte length (excluding \0) string ending with \0
	 */
	static const char* md5_string(const void *dat, size_t dlen,
		const void *key, size_t klen, char* out, size_t size);

	/**
	 * Calculate signature value of content in file using md5 algorithm, and get result in string form
	 * @param path {const char*} Full file path
	 * @param key {const char*} When not empty, used as key data
	 * @param klen {size_t} When key is not empty, indicates length of key
	 * @param out {void*} Store md5 result
	 * @param size {size_t} out size, should be at least 33 bytes
	 * @return {int64) Returns length of file data read. Returns -1 in the following cases:
	 *  1) Failed to open file
	 *  2) No data read from file
	 *  3) out buffer size is less than 33 bytes
	 */
#if defined(_WIN32) || defined(_WIN64)
	static __int64 md5_file(const char* path, const void *key,
		size_t klen, char* out, size_t size);
#else
	static long long int md5_file(const char* path, const void *key,
		size_t klen, char* out, size_t size);
#endif

	/**
	 * Calculate signature value of content in file using md5 algorithm, and get result in string form
	 * @param in {istream&} Input file stream
	 * @param key {const char*} When not empty, used as key data
	 * @param klen {size_t} When key is not empty, indicates length of key
	 * @param out {void*} Store md5 result
	 * @param size {size_t} out size, should be at least 33 bytes
	 * @return {int64) Returns length of file data read. Returns -1 in the following cases:
	 *  1) No data read from input stream
	 *  2) out buffer size is less than 33 bytes
	 */
#if defined(_WIN32) || defined(_WIN64)
	static __int64 md5_file(istream& in, const void *key,
		size_t klen, char* out, size_t size);
#else
	static long long int md5_file(istream& in, const void *key,
		size_t klen, char* out, size_t size);
#endif

	/**
	 * Convert binary data to string data by converting one byte into two bytes.
	 * @param in {const void*} Address of input binary data
	 * @param len {size_t} Length of in
	 * @param out {char*} Store result in string form
	 * @param size {size_t} out memory size, should be at least len * 2 + 1,
	 *  last byte stores \0
	 * @return {const char*} Returns address storing result (i.e., out address),
	 *  and return value is a string ending with \0.
	 */
	static const char* hex_encode(const void *in, size_t len,
		char* out, size_t size);

private:
	unsigned int buf_[4];
	unsigned int bytes_[2];
	unsigned int in_[16];

	unsigned char digest_[16];
	unsigned char digest_s_[33];
};

}  // namespace acl

