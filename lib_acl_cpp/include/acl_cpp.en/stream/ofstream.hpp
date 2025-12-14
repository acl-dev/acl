#pragma once
#include "../acl_cpp_define.hpp"
#include "fstream.hpp"

namespace acl {

class ACL_CPP_API ofstream : public fstream
{
public:
	ofstream();
	virtual ~ofstream();

	/**
	 * Open file in write-only mode, create new file if it doesn't exist
	 * @param path {const char*} Filename
	 * @param otrunc {bool} If file exists, whether to clear the file when opening
	 * @return {bool} Whether successful
	 */
	bool open_write(const char* path, bool otrunc = true);

	/**
	 * Open file in append mode, create new file if it doesn't exist
	 * @param path {const char*} Filename
	 * @return {bool} Whether successful
	 */
	bool open_append(const char* path);
};

} // namespace acl
