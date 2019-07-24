#pragma once
#include "../acl_cpp_define.hpp"
#include "fstream.hpp"

namespace acl {

class string;

class ACL_CPP_API ifstream: public fstream
{
public:
	ifstream(void) {}
	virtual ~ifstream(void) {}

	/**
	 * 以只读方式打开已经存在的文件
	 * @param path {const char*} 文件名
	 * @return {bool} 打开文件是否成功
	 */
	bool open_read(const char* path);

	/**
	 * 从打开的文件流中加载该文件中的所有内容到用户指定缓冲区内
	 * @param s {string*} 用户缓冲区
	 * @return {bool} 是否成功
	 */
	bool load(string* s);
	bool load(string& s);

	/**
	 * 加载文件中的数据至用户指定缓冲区, 该函数是静态成员变量，
	 * 可直接使用
	 * @param path {const char*} 文件名
	 * @param s {string*} 用户缓冲区
	 * @return {bool} 是否成功
	 */
	static bool load(const char* path, string* s);
	static bool load(const char* path, string& s);
};

} // namespace acl
