#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <list>
#include <string>
#include <stdarg.h>
#include <utility>
#include <iostream>

struct ACL_VSTRING;
struct ACL_LINE_STATE;

namespace acl {

class dbuf_pool;

/**
 * This is a string buffer class that supports most functions in std::string, and also supports some
 * functions that std::string does not support. Additionally, internally automatically ensures the last character is \0.
 */
class ACL_CPP_API string {
public:
	/**
	 * Constructor
	 * @param n {size_t} Initial allocated memory size.
	 * @param bin {bool} Whether to automatically use binary mode. When this value is true,
	 *  when calling += int|int64|short|char or << int|int64|short|char
	 *  operations, it will append in binary mode. Otherwise, it will append in text format.
	 */
	string(size_t n, bool bin);
	explicit string(size_t n);
	string();

	/**
	 * Constructor
	 * @param s {const string&} Source string object. Initializes buffer. Internally automatically copies
	 *  string.
	 */
	string(const string& s);

	/**
	 * Constructor
	 * @param s {const char*} Internally automatically uses this string to initialize buffer. s must
	 *  end with \0.
	 */
	string(const char* s);

	/**
	 * Constructor
	 * @param s {const char*} Source buffer address.
	 * @param n {size_t} Data length of s buffer.
	 */
	string(const void* s, size_t n);

	/**
	 * Constructor using memory-mapped file format.
	 * @param fd {int} File descriptor.
	 * @param max {size_t} Maximum mapped buffer space size.
	 * @param n {size_t} Initial buffer size.
	 * @param offset {size_t} Start position in memory-mapped file.
	 */
#if defined(_WIN32) || defined(_WIN64)
	string(void* fd, size_t max, size_t n, size_t offset = 0);
#else
	string(int fd, size_t max, size_t n, size_t offset = 0);
#endif

	~string();

	/**
	 * Set string buffer to binary append mode.
	 * @param bin {bool} When this value is true, sets string buffer to binary append
	 *  mode. Otherwise, it is text format. When true, when calling += int|int64|short|char
	 *  or << int|int64|short|char operations, it will append in binary mode. Otherwise, it will append in text
	 *  format.
	 * @return {string&}
	 */
	string& set_bin(bool bin);

	/**
	 * Set user-defined maximum length to avoid buffer overflow.
	 * @param max {int}
	 * @return {string&}
	 */
	string& set_max(int  max);

	/**
	 * Determine whether current string buffer is in binary append mode.
	 * @return {bool} Return value is true to indicate binary mode.
	 */
	bool get_bin() const;

	/**
	 * Get current buffer's maximum length limit. When return value <= 0, it means no limit.
	 * @return {int}
	 */
	int get_max() const;

	/**
	 * Get character at specified position in string buffer by subscript. When subscript value is valid, internally
	 * automatically checks bounds.
	 * @param n {size_t} Specified position. This value >= 0 and < string buffer length). Cannot exceed
	 *  bounds, otherwise error occurs.
	 * @return {char} Character at specified position.
	 */
	char operator[](size_t n) const;

	/**
	 * Get character at specified position in string buffer by subscript. When subscript value is valid, internally
	 * automatically checks bounds.
	 * @param n {int} Specified position. This value >= 0 and < string buffer length). Cannot exceed bounds,
	 *  otherwise error occurs.
	 * @return {char} Character at specified position.
	 */
	char operator[](int n) const;

	/**
	 * Assignment value operator. Users can directly use this object's subscript for assignment. When subscript value
	 * exceeds bounds, internally automatically expands buffer space.
	 * @param n {size_t} Target subscript position.
	 * @return {char&}
	 */
	char& operator[](size_t n);

	/**
	 * Assignment value operator. Users can directly use this object's subscript for assignment. When subscript value
	 * exceeds bounds, internally automatically expands buffer space.
	 * @param n {int} Target subscript position. This value must be >= 0.
	 * @return {char&}
	 */
	char& operator[](int n);

	/**
	 * Assign value to target string buffer.
	 * @param s {const char*} Source string.
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(const char* s);

	/**
	 * Assign value to target string buffer.
	 * @param s {const string&} Source string object.
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(const string& s);

	/**
	 * Assign value to target string buffer.
	 * @param s {const string*} Source string object pointer.
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(const string* s);

	/**
	 * Assign value to target string buffer.
	 * @param s {const std::string&} Source string object.
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(const std::string& s);

	/**
	 * Assign value to target string buffer.
	 * @param s {const std::string*} Source string object pointer.
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(const std::string* s);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Assign value to target string buffer.
	 * @param n {long long int} Source 64-bit signed integer. When current buffer's current state is
	 *  binary mode, this function automatically appends value to string buffer in binary mode. Otherwise, it appends in text
	 *  format. For details about binary mode and text format, see
	 *  set_bin(bool)
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(__int64 n);

	/**
	 * Assign value to target string buffer.
	 * @param n {unsinged long long int} Source 64-bit unsigned integer. When string buffer's
	 *  current state is binary mode, this function automatically appends value to string buffer
	 *  in binary mode. Otherwise, it appends in text format. For details about binary mode and text format, see
	 *  set_bin(bool)
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(unsigned __int64);
#else
	string& operator=(long long int);
	string& operator=(unsigned long long int);
#endif

	/**
	 * Assign value to target string buffer.
	 * @param n {char} Source signed character. When string buffer's current state is binary mode,
	 *  this function automatically appends value to string buffer in binary mode. Otherwise, it appends in text format. For
	 *  details about binary mode and text format, see set_bin(bool)
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(char n);

	/**
	 * Assign value to target string buffer.
	 * @param n {char} Source unsigned character. When current state is binary mode, this function
	 *  automatically appends value to string buffer in binary mode. Otherwise, it appends in text format to string buffer.
	 *  For details about binary mode and text format, see set_bin(bool)
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(unsigned char n);

	/**
	 * Assign value to target string buffer.
	 * @param n {char} Source signed long integer. When current state is binary mode, this function
	 *  automatically appends value to string buffer in binary mode. Otherwise, it appends in text format to string buffer.
	 *  For details about binary mode and text format, see set_bin(bool)
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(long n);

	/**
	 * Assign value to target string buffer.
	 * @param n {char} Source unsigned long integer. When current state is binary mode,
	 *  this function automatically appends value to string buffer in binary mode. Otherwise, it appends in text format to
	 *  string buffer. For details about binary mode and text format, see set_bin(bool)
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(unsigned long n);

	/**
	 * Assign value to target string buffer.
	 * @param n {char} Source signed integer. When string buffer's current state is binary mode,
	 *  this function automatically appends value to string buffer in binary mode. Otherwise, it appends in text format to string
	 *  buffer. For details about binary mode and text format, see set_bin(bool)
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(int n);

	/**
	 * Assign value to target string buffer.
	 * @param n {char} Source unsigned integer. When string buffer's current state is binary mode,
	 *  this function automatically appends value to string buffer in binary mode. Otherwise, it appends in text format to string
	 *  buffer. For details about binary mode and text format, see set_bin(bool)
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(unsigned int n);

	/**
	 * Assign value to target string buffer.
	 * @param n {char} Source signed short integer. When string buffer's current state is binary mode,
	 *  this function automatically appends value to string buffer in binary mode. Otherwise, it appends in text format to string
	 *  buffer. For details about binary mode and text format, see set_bin(bool)
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(short n);

	/**
	 * Assign value to target string buffer.
	 * @param n {char} Source unsigned short integer. When current state is binary mode, you
	 *  need to call this function to automatically append value to string buffer in binary mode. Otherwise, it appends in text format to string
	 *  buffer. For details about binary mode and text format, see set_bin(bool)
	 * @return {string&} Returns reference to current object, convenient for chaining operations on this object.
	 */
	string& operator=(unsigned short n);

	/**
	 * Append string to target string buffer at the end.
	 * @param s {const char*} Source string pointer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(const char* s);

	/**
	 * Append string to target string buffer at the end.
	 * @param s {const string&} Source string object reference.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(const string& s);

	/**
	 * Append string to target string buffer at the end.
	 * @param s {const string*} Source string object pointer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(const string* s);

	/**
	 * Append string to target string buffer at the end.
	 * @param s {const std::string&} Source string object reference.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(const std::string& s);

	/**
	 * Append string to target string buffer at the end.
	 * @param s {const std::string*} Source string object pointer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(const std::string* s);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Append signed integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long long int} Source 64-bit signed integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(__int64 n);

	/**
	 * Append unsigned integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long long int} Source 64-bit unsigned integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(unsigned __int64 n);
#else
	string& operator+=(long long int n);
	string& operator+=(unsigned long long int n);
#endif

	/**
	 * Append signed integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source signed long integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(long n);

	/**
	 * Append unsigned integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source unsigned long integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(unsigned long n);

	/**
	 * Append signed integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source signed integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(int n);

	/**
	 * Append unsigned integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source unsigned integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(unsigned int n);

	/**
	 * Append signed integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source signed short integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(short n);

	/**
	 * Append unsigned integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source unsigned short integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(unsigned short n);

	/**
	 * Append signed character to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source signed character.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(char n);

	/**
	 * Append unsigned character to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source unsigned character.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator+=(unsigned char n);

	/**
	 * Append string to target string buffer at the end.
	 * @param s {const string&} Source string object reference.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(const string& s);

	/**
	 * Append string to target string buffer at the end.
	 * @param s {const string*} Source string object pointer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(const string* s);

	/**
	 * Append string to target string buffer at the end.
	 * @param s {const std::string&} Source string object reference.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(const std::string& s);

	/**
	 * Append string to target string buffer at the end.
	 * @param s {const std::string*} Source string object pointer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(const std::string* s);

	/**
	 * Append string to target string buffer at the end.
	 * @param s {const char*} Source string pointer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(const char* s);
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Append signed integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long long int} Source 64-bit signed integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(__int64 n);

	/**
	 * Append unsigned integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long long int} Source 64-bit unsigned integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(unsigned __int64 n);
#else
	string& operator<<(long long int n);
	string& operator<<(unsigned long long int n);
#endif

	/**
	 * Append signed integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source signed long integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(long n);

	/**
	 * Append unsigned integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source unsigned long integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(unsigned long n);

	/**
	 * Append signed integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source signed integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(int n);

	/**
	 * Append unsigned integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source unsigned integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(unsigned int n);

	/**
	 * Append signed integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source signed short integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(short n);

	/**
	 * Append unsigned integer number to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source unsigned short integer.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(unsigned short n);

	/**
	 * Append signed character to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source signed character.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(char n);

	/**
	 * Append unsigned character to target string buffer at the end. When target string buffer is
	 * binary mode, it appends in binary character format. Otherwise, it appends in text format.
	 * @param n {long} Source unsigned character.
	 * @return {string&} Target string buffer reference.
	 */
	string& operator<<(unsigned char n);

	/**
	 * Copy data from string buffer to target string buffer.
	 * @param s {string*} Target string buffer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(string* s);

	/**
	 * Copy data from string buffer to target string buffer.
	 * @param s {string&} Target string buffer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(string& s);

	/**
	 * Copy data from string buffer to target string buffer.
	 * @param s {std::string*} Target string buffer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(std::string* s);

	/**
	 * Copy data from string buffer to target string buffer.
	 * @param s {std::string&} Target string buffer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(std::string& s);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Copy data from string buffer to target 64-bit signed integer.
	 * @param n {string*} Target 64-bit signed integer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(__int64& n);

	/**
	 * Copy data from string buffer to target 64-bit unsigned integer.
	 * @param n {string*} Target 64-bit unsigned integer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(unsigned __int64& n);
#else
	size_t operator>>(long long int&);
	size_t operator>>(unsigned long long int&);
#endif

	/**
	 * Copy data from string buffer to target 32-bit signed integer.
	 * @param n {string*} Target 32-bit signed integer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(int& n);

	/**
	 * Copy data from string buffer to target 32-bit unsigned integer.
	 * @param n {string*} Target 32-bit unsigned integer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(unsigned int& n);

	/**
	 * Copy data from string buffer to target 16-bit signed integer.
	 * @param n {string*} Target 16-bit signed integer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(short& n);

	/**
	 * Copy data from string buffer to target 16-bit unsigned integer.
	 * @param n {string*} Target 16-bit unsigned integer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(unsigned short& n);

	/**
	 * Copy data from string buffer to target 8-bit signed character.
	 * @param n {string*} Target 16-bit signed character.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(char& n);

	/**
	 * Copy data from string buffer to target 8-bit unsigned character.
	 * @param n {string*} Target 16-bit unsigned character.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t operator>>(unsigned char& n);

	/**
	 * Determine whether current buffer's stored data and target string buffer's stored data are equal. Internally case-sensitive.
	 * @param s {const string&} Target string buffer object reference.
	 * @return {bool} Returns true to indicate string data is the same.
	 */
	bool operator==(const string& s) const;

	/**
	 * Determine whether current buffer's stored data and target string buffer's stored data are equal. Internally case-sensitive.
	 * @param s {const string&} Target string buffer object pointer.
	 * @return {bool} Returns true to indicate string data is the same.
	 */
	bool operator==(const string* s) const;

	/**
	 * Determine whether current buffer's stored data and target string are equal. Internally case-sensitive.
	 * @param s {const string&} Target string buffer object pointer.
	 * @return {bool} Returns true to indicate string data is the same.
	 */
	bool operator==(const char* s) const;

	/**
	 * Determine whether current buffer's stored data and target string buffer's stored data are not equal. Internally case-sensitive.
	 * @param s {const string&} Target string buffer object reference.
	 * @return {bool} Returns true to indicate string data is different.
	 */
	bool operator!=(const string& s) const;

	/**
	 * Determine whether current buffer's stored data and target string buffer's stored data are not equal. Internally case-sensitive.
	 * @param s {const string&} Target string buffer object pointer.
	 * @return {bool} Returns true to indicate string data is different.
	 */
	bool operator!=(const string* s) const;

	/**
	 * Determine whether current buffer's stored data and target string are not equal. Internally case-sensitive.
	 * @param s {const string&} Target string buffer object pointer.
	 * @return {bool} Returns true to indicate string data is different.
	 */
	bool operator!=(const char* s) const;

	/**
	 * Determine whether current buffer's data is less than target string buffer's stored data. Internally case-sensitive.
	 * @param s {const string&} Target string buffer object reference.
	 * @return {bool} Returns true to indicate current string buffer's stored data is less than target string
	 *  buffer's stored data.
	 */
	bool operator<(const string& s) const;

	/**
	 * Determine whether current buffer's data is greater than target string buffer's stored data. Internally case-sensitive.
	 * @param s {const string&} Target string buffer object reference.
	 * @return {bool} Returns true to indicate current string buffer's stored data is greater than target string
	 *  buffer's stored data.
	 */
	bool operator>(const string& s) const;

	/**
	 * Calculate hash value of current string. Only for use with std::unordered_xxx containers in C++11.
	 * @return {size_t}
	 */
	size_t hash() const;

	/**
	 * Convert current buffer directly to string pointer (can directly call internally).
	 * @return {const char*} Return value is always non-empty pointer, but may be empty string.
	 */
	operator const char*() const;

	/**
	 * Convert current string buffer directly to generic pointer (can directly call internally).
	 * @return {const char*} Return value is always non-empty pointer.
	 */
	operator const void*() const;

	/**
	 * Convert acl::string to std::string
	 * @return {const std::string}
	 */
	operator const std::string() const;

	/**
	 * Add a signed character to the end of current string buffer.
	 * @param ch {char} Signed character.
	 * @param term {bool} Whether to append \0 at the end. Appended \0 will increase length.
	 * @return {string&} Current string buffer reference.
	 */
	string& push_back(char ch, bool term = true);
	string& push_back(unsigned char ch, bool term = true);

	/**
	 * Get last character in string buffer. Returns -1 when string is empty.
	 * @return {char} Returns last character, -1.
	 */
	char back() const;

	/**
	 * Remove last character.
	 * @return {bool} When string is not empty, after removing last character, returns true. Otherwise returns false.
	 */
	bool pop_back();

	/**
	 * Append \0 at buffer end for safety when using. Appended \0 will increase length.
	 * @return {string&} Current string buffer reference.
	 */
	string& terminate();

	/**
	 * Compare whether two string buffers' stored data are the same. Case-sensitive.
	 * @param s {const string&} Target string buffer object reference.
	 * @param case_sensitive {bool} When true, means case-sensitive.
	 * @return {bool} Returns true to indicate they are equal.
	 */
	bool equal(const string& s, bool case_sensitive = true) const;

	/**
	 * Check whether current string object starts with specified string.
	 * @param s {const char*}
	 * @param case_sensitive {bool} Whether case-sensitive.
	 * @return {bool}
	 */
	bool begin_with(const char* s, bool case_sensitive = true) const;
	bool begin_with(const char* s, size_t len, bool case_sensitive = true) const;
	bool begin_with(const string& s, bool case_sensitive = true) const;
	bool begin_with(const void* v, size_t len) const;

	/**
	 * Check whether current string object ends with specified string.
	 * @param s {const char*}
	 * @param case_sensitive {bool} Whether case-sensitive.
	 * @return {bool}
	 */
	bool end_with(const char* s, bool case_sensitive = true) const;
	bool end_with(const char* s, size_t len, bool case_sensitive = true) const;
	bool end_with(const string& s, bool case_sensitive = true) const;
	bool end_with(const void* v, size_t len) const;

	/**
	 * Compare whether two string buffers' stored data are the same. Case-sensitive.
	 * @param s {const string&} Target string buffer object reference.
	 * @return {int} 0: Indicates they are the same. > 0: Current string buffer's data is greater than target data.
	 *  < 0: Current string buffer's data is less than target data.
	 */
	int compare(const string& s) const;

	/**
	 * Compare whether two string buffers' stored data are the same. Case-sensitive.
	 * @param s {const string&} Target string buffer object pointer.
	 * @return {int} 0: Indicates they are the same. > 0: Current string buffer's data is greater than target data.
	 *  < 0: Current string buffer's data is less than target data.
	 */
	int compare(const string* s) const;

	/**
	 * Compare whether two strings are the same.
	 * @param s {const string&} Target string buffer object reference.
	 * @param case_sensitive {bool} When true, means case-sensitive.
	 * @return {int} 0: Indicates they are the same. > 0: Current string buffer's data is greater than target data.
	 *  < 0: Current string buffer's data is less than target data.
	 */
	int compare(const char* s, bool case_sensitive = true) const;

	/**
	 * Compare whether current buffer's buffer data and target buffer's buffer data are the same.
	 * @param ptr {const void*} Target buffer's buffer address.
	 * @param len {size_t} Data length of ptr buffer.
	 * @return {int} Returns comparison result:
	 *  0: Indicates they are the same.
	 *  > 0: Current buffer's buffer data is greater than target data.
	 *  < 0: Current buffer's buffer data is less than target data.
	 */
	int compare(const void* ptr, size_t len) const;

	/**
	 * Compare whether current buffer's buffer data and target buffer's buffer data are the same. Limits comparison data length.
	 * @param s {const void*} Target buffer's buffer address.
	 * @param len {size_t} Data length of ptr buffer.
	 * @param case_sensitive {bool} When true, means case-sensitive.
	 * @return {int} 0: Indicates they are the same. > 0: Current buffer's buffer data is greater than target data.
	 *  < 0: Current buffer's buffer data is less than target data.
	 */
	int ncompare(const char* s, size_t len, bool case_sensitive = true) const;

	/**
	 * Compare from end forward whether current buffer's buffer data and target buffer's buffer data are the same.
	 * Limits comparison data length.
	 * @param s {const void*} Target buffer's buffer address.
	 * @param len {size_t} Data length of ptr buffer.
	 * @param case_sensitive {bool} When true, means case-sensitive.
	 * @return {int} 0: Indicates they are the same.
	 *  > 0: Current buffer's buffer data is greater than target data.
	 *  < 0: Current buffer's buffer data is less than target data.
	 */
	int rncompare(const char* s, size_t len, bool case_sensitive = true) const;

	/**
	 * Find blank line position in current string buffer data. You can call this function in a loop to get all
	 * blank lines in buffer.
	 * @param left_count {int*} When pointer is not empty, stores remaining data length of current string.
	 * @param buf {string*} When blank line is found, stores one blank line (including blank line) or
	 *  next blank line (including blank line) and data between them in this buffer. Note: Internally automatically
	 *  clears this buffer, then uses append method.
	 * @return {int} Returns 0 to indicate blank line not found. Return value > 0 indicates blank line's first
	 *  position (because to find first blank line, returned is that blank line's first position. If not found,
	 *  return value is always 0). Return value < 0 indicates internal error.
	 */
	int find_blank_line(int* left_count = NULL, string* buf = NULL);

	/**
	 * Reset internal query state. When you need to start over, when calling find_blank_line, you need to call this
	 * function to reset internal query state.
	 * @return {string&}
	 */
	string& find_reset();

	/**
	 * Find specified character's position in current buffer buffer. Subscript starts from 0.
	 * @param n {char} Signed character to find.
	 * @return {int} Character's position in buffer. Return value < 0 indicates not found.
	 */
	int find(char n) const;

	/**
	 * Find specified string's start position in current buffer buffer. Subscript starts from 0.
	 * @param needle {const char*} Signed string to find.
	 * @param case_sensitive {bool} When true, means case-sensitive.
	 * @return {char*} String's start position in buffer. Returns empty pointer to indicate not found.
	 */
	char* find(const char* needle, bool case_sensitive=true) const;

	/**
	 * Find from end forward specified string's start position in current buffer buffer. Subscript starts from 0.
	 * @param needle {const char*} Signed string to find.
	 * @param case_sensitive {bool} When true, means case-sensitive.
	 * @return {char*} String's start position in buffer. Return value is empty pointer to indicate not found.
	 */
	char* rfind(const char* needle, bool case_sensitive=true) const;

	/**
	 * Return substring from current string buffer from start to specified position.
	 * @param n {size_t} Subscript position. When this value is greater than or equal to current string buffer's data length,
	 *  returns entire string buffer. Otherwise, returns string from start to specified position.
	 * @return {string} Return value is a new object, does not need to be manually released. This function's efficiency
	 *  may not be too high.
	 */
	string left(size_t n);

	/**
	 * Return substring from current string buffer from specified position to end.
	 * @param n {size_t} Subscript position. When this value is greater than or equal to current string buffer's data length,
	 *  returns string buffer object as empty. Otherwise, returns string from specified position to end.
	 * @return {const string} Return value is a new object, does not need to be manually released, but
	 *  this function's efficiency may not be too high.
	 */
	string right(size_t n);

	/**
	 * Copy a certain amount of data from current buffer buffer to target buffer.
	 * @param buf {void*} Target buffer address.
	 * @param size {size_t} Buffer size of buf.
	 * @param move {bool} After copying data, whether to move remaining data forward to
	 *  remove copied data from current buffer.
	 * @return {size_t} Returns actual copied byte count. Returns 0 when empty() == true.
	 */
	size_t scan_buf(void* buf, size_t size, bool move = false);

	/**
	 * Copy one line (excluding "\r\n") from current buffer buffer to target buffer. After
	 * copying, remaining data in target buffer and source buffer will move forward. This is a
	 * non-destructive operation.
	 * @param out {string&} Target buffer object. Internally automatically clears this buffer.
	 * @param nonl {bool} Whether to remove "\r\n" or "\n" at the end when returning first line.
	 * @param n {size_t*} When this parameter is non-empty pointer, stores length of copied line data. When
	 *  reading one line and nonl is true, address stores 0.
	 * @param move {bool} After copying data, whether to move remaining data forward to
	 *  remove copied data from current buffer.
	 * @return {bool} Whether one line of data was copied. When this function returns false, you need to check
	 *  empty() == true to determine whether current buffer is empty.
	 */
	bool scan_line(string& out, bool nonl = true, size_t* n = NULL,
		bool move = false);

	/**
	 * When using scan_xxx series functions to operate on buffer data without specifying move parameter, you can call this
	 * function to move remaining data forward to buffer bottom.
	 * @return {size_t} Number of bytes moved.
	 */
	size_t scan_move();

	/**
	 * Get end address of first line of data in current buffer buffer.
	 * @return {char*} Return value is NULL to indicate internal buffer is empty, i.e., empty() == true.
	 */
	char* buf_end();

	/**
	 * Get start address of current buffer buffer.
	 * @return {void*} Returned address is always non-empty.
	 */
	void* buf() const;

	/**
	 * Get start address of current buffer buffer in string format.
	 * @return {char*} Returned address is always non-empty.
	 */
	char* c_str() const;

	/**
	 * Get length of current stored string, excluding \0.
	 * @return {size_t} Return value >= 0.
	 */
	size_t length() const;

	/**
	 * Get length of current stored string, excluding \0. Same as length.
	 * @return {size_t} Return value >= 0.
	 */
	size_t size() const;

	/**
	 * Get space length of current buffer buffer. This value >= stored data length.
	 * @return {size_t} Return value > 0.
	 */
	size_t capacity() const;

	/**
	 * Determine whether current buffer buffer's data length is 0.
	 * @return {bool} Returns true to indicate buffer is empty.
	 */
	bool empty() const;

	/**
	 * Get ACL_VSTRING object pointer used internally in current object in acl C library.
	 * @return {ACL_VSTRING*} Return value is always non-empty.
	 */
	ACL_VSTRING* vstring() const;

	/**
	 * Set current buffer buffer's subscript position to specified position.
	 * @param n {size_t} Target subscript position. When this value >= capacity, internally
	 *  will reallocate some memory.
	 * @return {string&} Current object reference.
	 */
	string& set_offset(size_t n);

	/**
	 * Call this function to pre-allocate required buffer size.
	 * @param n {size_t} Desired buffer space minimum size.
	 * @return {string&} Current object reference.
	 */
	string& space(size_t n);
	string& reserve(size_t n);

	/**
	 * Split string stored in current buffer.
	 * @param sep {const char*} Separator string when splitting. Each character in this string
	 *  can be used as separator. Note: Internally automatically escapes special characters in string when splitting.
	 * @param quoted {bool} When true, when source string has single/double quotes wrapping string data,
	 *  it will not split. Note: Separator string sep should not contain single/double quotes. Source is:
	 *  "abc*|\"cd*ef\"|\'gh|ijk\'|lmn", separator string is "*|". When quoted is true,
	 *  split result is: "abc", "cd*ef", "gh|ijk", "lmn". When quoted is false,
	 *  split result is: "abc", "cd", "ef", "gh", ijk", "lmn"
	 * @return {std::list<string>&} Returns list format split result. Returned
	 *  result does not need to be released, because it is an internal pointer of current object.
	 */
	std::list<string>& split(const char* sep, bool quoted = false);

	/**
	 * Split string stored in current buffer.
	 * @param sep {const char*} Separator string when splitting. Same as split.
	 * @param quoted {bool} When true, when source string has single/double quotes wrapping
	 *  string data, it will not split. Note: sep should not contain single/double quotes.
	 * @return {std::vector<string>&} Returns vector format split result.
	 *  Returned result does not need to be released, because it is an internal pointer of current object.
	 */
	std::vector<string>& split2(const char* sep, bool quoted = false);

	/**
	 * Use '=' as separator to split string stored in current buffer into name/value pairs. When splitting,
	 * automatically removes spaces TAB at start and end of source string and around separator '='.
	 * @param sep {char} Users can specify their own separator through this parameter.
	 * @return {std::pair<string, string>&} When current buffer stores string
	 *  format is legal (name=value format), returns split result. Otherwise, returned result's string
	 *  content is empty. Returned result does not need to be released, because it is an internal string of current object.
	 */
	std::pair<string, string>& split_nameval(char sep = '=');

	/**
	 * Start from left side of string, search forward for specified delimiter, and cut at right side of delimiter if found. This function's difference from split_nameval
	 * is that it will not remove spaces/TAB characters on left and right sides of string, and right side will also remove delimiter at the beginning.
	 * @param delimiter {char} Delimiter.
	 * @return {std::pair<string, string>&} Returns name, value pair. When delimiter is not found,
	 *  returned std::pair's both are empty.
	 */
	std::pair<string, string>& split_at(char delimiter);

	/**
	 * Start from right side of string, search forward for specified delimiter, and cut at right side if found.
	 * @param delimiter {char} Delimiter.
	 * @return {char*} Delimiter's found character. Returns NULL when specified delimiter is not found.
	 */
	std::pair<string, string>& split_at_right(char delimiter);

	/**
	 * Copy string data to current buffer buffer.
	 * @param ptr {const char*} Source string address. Must end with '\0'.
	 * @return {string&} Current object reference.
	 */
	string& copy(const char* ptr);

	/**
	 * Copy source data's binary data to current buffer buffer.
	 * @param ptr {const void*} Source data address.
	 * @param len {size_t} Data length of ptr source data.
	 * @return {string&} Current object reference.
	 */
	string& copy(const void* ptr, size_t len);

	/**
	 * Move source string data to current buffer buffer. Internally automatically determines whether source data
	 * address is within current buffer buffer.
	 * @param src {const char*} Source data address.
	 * @return {string&} Current object reference.
	 */
	string& memmove(const char* src);

	/**
	 * Move source string data to current buffer buffer. Internally automatically determines whether source data
	 * address is within current buffer buffer.
	 * @param src {const char*} Source data address.
	 * @param len {size_t} Length of moved data.
	 * @return {string&} Current object reference.
	 */
	string& memmove(const char* src, size_t len);

	/**
	 * Append specified string data to current buffer data at the end.
	 * @param s {const string&} Source data object reference.
	 * @return {string&} Current object reference.
	 */
	string& append(const string& s);

	/**
	 * Append specified string data to current buffer data at the end.
	 * @param s {const string&} Source data object pointer.
	 * @return {string&} Current object reference.
	 */
	string& append(const string* s);

	/**
	 * Append specified string data to current buffer data at the end.
	 * @param s {const string&} Source data object pointer.
	 * @return {string&} Current object reference.
	 */
	string& append(const char* s);

	/**
	 * Append specified memory buffer's data to current buffer data at the end.
	 * @param ptr {const void*} Source data object pointer.
	 * @param len {size_t} Data length of ptr.
	 * @return {string&} Current object reference.
	 */
	string& append(const void* ptr, size_t len);

	/**
	 * Prepend specified string data to current buffer data at the beginning.
	 * @param s {const char*} Source data address.
	 * @return {string&} Current object reference.
	 */
	string& prepend(const char* s);

	/**
	 * Prepend specified memory buffer's data to current buffer data at the beginning.
	 * @param ptr {const void*} Source data address.
	 * @param len {size_t} Data length of ptr.
	 * @return {string&} Current object reference.
	 */
	string& prepend(const void* ptr, size_t len);

	/**
	 * Insert memory data at specified subscript position starting from current buffer buffer.
	 * @param start {size_t} Start subscript value of current buffer buffer.
	 * @param ptr {const void*} Memory data address.
	 * @param len {size_t} Length of memory data.
	 * @return {string&} Current object reference.
	 */
	string& insert(size_t start, const void* ptr, size_t len);

	/**
	 * Format and assign data, similar to sprintf interface format.
	 * @param fmt {const char*} Format string.
	 * @param ... Variable arguments.
	 * @return {string&} Current object reference.
	 */
	string& format(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * Format and assign data, similar to vsprintf interface format.
	 * @param fmt {const char*} Format string.
	 * @param ap {va_list} Variable arguments.
	 * @return {string&} Current object reference.
	 */
	string& vformat(const char* fmt, va_list ap);

	/**
	 * Format and append data at current buffer end.
	 * @param fmt {const char*} Format string.
	 * @param ... Variable arguments.
	 * @return {string&} Current object reference.
	 */
	string& format_append(const char* fmt, ...)  ACL_CPP_PRINTF(2, 3);

	/**
	 * Format and append data at current buffer end.
	 * @param fmt {const char*} Format string.
	 * @param ap {va_list} Variable arguments.
	 * @return {string&} Current object reference.
	 */
	string& vformat_append(const char* fmt, va_list ap);

	/**
	 * Replace character data in current buffer with target character.
	 * @param from {char} Source character.
	 * @param to {char} Target character.
	 * @return {string&} Current object reference.
	 */
	string& replace(char from, char to);

	/**
	 * Truncate current buffer data, and internally moves subscript pointer forward.
	 * @param n {size_t} Data length after truncation. When this value >= current buffer's
	 *  data length, internally makes no changes.
	 * @return {string&} Current object reference.
	 */
	string& truncate(size_t n);

	/**
	 * In current buffer buffer, remove specified character string data. Memory data in buffer will
	 * move forward.
	 * @param needle {const char*} Specified character string to remove.
	 * @param each {bool} When true, for each character that appears in needle string,
	 *  remove it from current buffer buffer. Otherwise, remove needle string from current buffer buffer.
	 * @return {string&} Current object reference.
	 *  Example: acl::string s("hello world!");
	 *  Call s.strip("hel", true), result is: s == "o word!"
	 *  Call s.strip("hel", false), result is: s = "lo world!"
	 */
	string& strip(const char* needle, bool each = false);

	/**
	 * Remove whitespace (spaces TAB) on left side of current buffer buffer.
	 * @return {string&} Current object reference.
	 */
	string& trim_left_space();

	/**
	 * Remove whitespace (spaces TAB) on right side of current buffer buffer.
	 * @return {string&} Current object reference.
	 */
	string& trim_right_space();

	/**
	 * Remove all whitespace (spaces TAB) in current buffer buffer.
	 * @return {string&} Current object reference.
	 */
	string& trim_space();

	/**
	 * Remove carriage return and line feed on left side of current buffer buffer.
	 * @return {string&} Current object reference.
	 */
	string& trim_left_line();

	/**
	 * Remove carriage return and line feed on right side of current buffer buffer.
	 * @return {string&} Current object reference.
	 */
	string& trim_right_line();

	/**
	 * Remove all carriage return and line feed in current buffer buffer.
	 * @return {string&} Current object reference.
	 */
	string& trim_line();

	/**
	 * Clear current buffer data buffer.
	 * @return {string&} Current object reference.
	 */
	string& clear();

	/**
	 * Convert all data in current buffer data buffer to lowercase.
	 * @return {string&} Current object reference.
	 */
	string& lower();

	/**
	 * Convert all data in current buffer data buffer to uppercase.
	 * @return {string&} Current object reference.
	 */
	string& upper();

	/**
	 * Copy data of specified offset and specified length from current buffer to target buffer.
	 * @param out {string&} Target buffer. Internally uses append method, and automatically clears this object.
	 * @param p {size_t} Start position in current buffer.
	 * @param len {size_t} Number of bytes to copy starting from position p. When this value is 0,
	 *  copies all data from position p. Otherwise, copies specified length of data. When specified
	 *  data length is greater than actual available length, copies actual existing data.
	 * @return {size_t} Returns actual copied data length. When p exceeds bounds, return value is 0.
	 */
	size_t substr(string& out, size_t p = 0, size_t len = 0) const;

	/**
	 * Encode all data in current buffer data buffer with base64 encoding.
	 * @return {string&} Current object reference.
	 */
	string& base64_encode();

	/**
	 * Encode source data with base64 encoding and append to current buffer buffer.
	 * @param ptr {const void*} Source data address.
	 * @param len {size_t} Source data length.
	 * @return {string&} Current object reference.
	 */
	string& base64_encode(const void* ptr, size_t len);

	/**
	 * When current buffer buffer's data is base64 encoded, this function decodes these
	 * data.
	 * @return {string&} Current object reference. When decoding fails, internal buffer will be automatically cleared,
	 *  so string::empty() returns true.
	 */
	string& base64_decode();

	/**
	 * Decode base64 encoded source data and append to current buffer buffer.
	 * @param s {const char*} Base64 encoded source data.
	 * @return {string&} Current object reference. When decoding fails, internal buffer will be automatically cleared,
	 *  so string::empty() returns true.
	 */
	string& base64_decode(const char* s);

	/**
	 * Decode base64 encoded source data and append to current buffer buffer.
	 * @param ptr {const void*} Base64 encoded source data.
	 * @param len {size_t} Data length of ptr.
	 * @return {string&} Current object reference. When decoding fails, internal buffer will be automatically cleared,
	 *  so string::empty() returns true.
	 */
	string& base64_decode(const void* ptr, size_t len);

	/**
	 * Encode source data with url encoding and append to current buffer buffer.
	 * @param s {const char*} Source data.
	 * @param dbuf {dbuf_pool*} Memory pool object. When not empty, internal dynamic memory will
	 *  be allocated from this object. When this object is released, internal temporary dynamic memory will also be released.
	 *  Otherwise, uses acl_mymalloc to allocate and automatically release.
	 * @return {string&} Current object reference.
	 */
	string& url_encode(const char* s, dbuf_pool* dbuf = NULL);

	/**
	 * Decode url encoded source data and append to current buffer buffer.
	 * @param s {const char*} Url encoded source data.
	 * @param dbuf {dbuf_pool*} Memory pool object. When not empty, internal dynamic memory will
	 *  be allocated from this object. When this object is released, internal temporary dynamic memory will also be released.
	 * @return {string&} Current object reference.
	 */
	string& url_decode(const char* s, dbuf_pool* dbuf = NULL);

	/**
	 * Encode source data with H2B encoding and append to current buffer buffer.
	 * @param s {const void*} Source data address.
	 * @param len {size_t} Source data length.
	 * @return {string&} Current object reference.
	 */
	string& hex_encode(const void* s, size_t len);

	/**
	 * Decode source data with H2B encoding and append to current buffer buffer.
	 * @param s {const char*} Source data address.
	 * @param len {size_t} Source data length.
	 * @return {string&} Current object reference.
	 */
	string& hex_decode(const char* s, size_t len);

	/**
	 * Extract file name from file full path.
	 * @param path {const char*} File full path string, cannot be empty string.
	 * @return {string&} Current object reference.
	 */
	string& basename(const char* path);

	/**
	 * Extract file's directory from file full path.
	 * @param path {const char*} File full path string, cannot be empty string.
	 * @return {string&} Current object reference.
	 */
	string& dirname(const char* path);

	/**
	 * Convert 32-bit signed integer to string reference (internally uses thread-local storage).
	 * @param n {int} 32-bit signed integer.
	 * @return {string&} Converted result reference. This reference is valid, because internally uses a thread-local buffer.
	 */
	static string& parse_int(int n);

	/**
	 * Convert 32-bit unsigned integer to string reference (internally uses thread-local storage).
	 * @param n {int} 32-bit unsigned integer.
	 * @return {string&} Converted result reference. This reference is valid, because internally uses a thread-local buffer.
	 */
	static string& parse_int(unsigned int n);
#if defined(_WIN32) || defined(_WIN64)
	static string& parse_int64(__int64 n);
	static string& parse_int64(unsigned __int64 n);
#else
	/**
	 * Convert 64-bit signed integer to string reference (internally uses thread-local storage).
	 * @param n {long long int} 64-bit signed integer.
	 * @return {string&} Converted result reference. This reference is valid, because internally uses a thread-local buffer.
	 */
	static string& parse_int64(long long int n);

	/**
	 * Convert 64-bit unsigned integer to string reference (internally uses thread-local storage).
	 * @param n {unsigned long long int} 64-bit unsigned integer.
	 * @return {string&} Converted result reference. This reference is valid, because internally uses a thread-local buffer.
	 */
	static string& parse_int64(unsigned long long int n);
#endif

	/**
	 * Template function, supports the following code:
	 * string s1, s2;
	 * T v;
	 * s1 = s2 + v;
	 */
	template<typename T>
	string operator + (T v) {
		string s(*this);
		s += v;
		return s;
	}

private:
	ACL_VSTRING* vbf_;
	char* scan_ptr_;
	std::list<string>* list_tmp_;
	std::vector<string>* vector_tmp_;
	std::pair<string, string>* pair_tmp_;
	ACL_LINE_STATE* line_state_;
	int  line_state_offset_;
	bool use_bin_;

	void init(size_t len);
};

/**
 * string s = "ok";
 * printf("first: %s\r\n", "ok" == s ? "true" : "false");
 */
bool operator == (const string* s, const string& str);
bool operator == (const char* s, const string& str);

/**
 * Template function, supports the following code:
 * string s1, s2;
 * T v;
 * s1 = v + s2;
 */
template<typename T>
string operator + (T v, const string& rhs) {
	string s;
	s = v;
	s += rhs;
	return s;
}

/**
 * Examples:
 * string s, s1 = "hello", s2 = "world";
 * s = s1 + " " + s2;
 * s = ">" + s1 + " " + s2;
 * s = 1000 + s1 + " " + s2 + 1000;
 */

// acl::string s1 = "hello world!";
// std::string s2;
// s2 += s1;
std::string& operator += (std::string& l, const acl::string& r);

// std::string s1;
// acl::string s2;
// if (s1 == s2) {}
bool operator == (const std::string& l, const acl::string& r);

// acl::string s1;
// std::string s2;
// if (s1 == s2) {}
bool operator == (const acl::string& l, const std::string& r);

// acl::string s = "hello world!";
// std::cout << s << std::endl;
std::ostream& operator << (std::ostream& o, const acl::string& s);

/**
 * Split string.
 * @param str {const char*} Source string to split.
 * @param sep {const char*} Separator string. Each character in this string can be used as separator.
 * @param out {std::vector<std::string>&} Store split string collection.
 */
void split(const char* str, const char* sep, std::vector<std::string>& out);

/**
 * Split string.
 * @param str {const char*} Source string to split.
 * @param sep {const char*} Separator string. Each character in this string can be used as separator.
 * @param out {std::list<std::string>&} Store split string collection.
 * @return {size_t} Returns number of split string elements.
 */
size_t split(const char* str, const char* sep, std::list<std::string>& out);

} // namespce acl

#if __cplusplus >= 201103L      // Support c++11 ?

namespace std {

// Hash function, only for use with std::unordered_xxx containers in C++11.

template <>
struct hash<acl::string> {
	size_t operator()(const acl::string& key) const {
		return key.hash();
	}
};

} // namespace std

#endif	// __cplusplus >= 201103L

