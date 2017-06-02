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
	 * 以只写方式打开文件，如果文件不存在则创建新文件，如果文件
	 * 存在，则将文件内容清空
	 * @param path {const char*} 文件名
	 * @return {bool} 是否成功
	 */
	bool open_write(const char* path);

	/**
	 * 以尾部添加方式打开文件，如果文件不存在则创建新文件
	 * @param path {const char*} 文件名
	 * @return {bool} 是否成功
	 */
	bool open_append(const char* path);
};

} // namespace acl
