#pragma once
#include "../acl_cpp_define.hpp"
#include "fstream.hpp"

namespace acl {

class string;

class ACL_CPP_API ifstream: public fstream {
public:
	ifstream() {}
	virtual ~ifstream() {}

	/**
	 * Open existing file in read-only mode.
	 * @param path {const char*} File path.
	 * @return {bool} Whether file opening was successful.
	 */
	bool open_read(const char* path);

	/**
	 * Read all data from opened file and store in user-specified buffer.
	 * @param s {string*} User buffer.
	 * @return {bool} Whether successful.
	 */
	bool load(string* s);
	bool load(string& s);

	/**
	 * Read all data from file into user-specified buffer. This function is a static member function
	 * and can be used directly.
	 * @param path {const char*} File path.
	 * @param s {string*} User buffer.
	 * @return {bool} Whether successful.
	 */
	static bool load(const char* path, string* s);
	static bool load(const char* path, string& s);
};

} // namespace acl
