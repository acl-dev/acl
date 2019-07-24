#pragma once
#include "../acl_cpp_define.hpp"
#include "fstream.hpp"

namespace acl {

class ACL_CPP_API ofstream : public fstream
{
public:
	ofstream(void);
	virtual ~ofstream(void);

	/**
	 * 以只写方式打开文件，如果文件不存在则创建新文件
	 * @param path {const char*} 文件名
	 * @param otrunc {bool} 若文件存在，则打开文件时是否需要先清空该文件
	 * @return {bool} 是否成功
	 */
	bool open_write(const char* path, bool otrunc = true);

	/**
	 * 以尾部添加方式打开文件，如果文件不存在则创建新文件
	 * @param path {const char*} 文件名
	 * @return {bool} 是否成功
	 */
	bool open_append(const char* path);
};

} // namespace acl
