#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <vector>

#ifndef ACL_CLIENT_ONLY

namespace acl {

class string;
class hsrow;

class ACL_CPP_API hsproto : public noncopyable
{
public:
	hsproto(bool cache_enable);
	~hsproto();

	/**
	 * 创建打开数据库索引的请求协议数据
	 * @param out {string&} 存储请求协议结果
	 * @param id {int} 对应打开索引的表ID号
	 * @param dbn {const char*} 数据库名称
	 * @param tbl {const char*} 数据库表名
	 * @param idx {const char*} 索引字段名
	 * @param flds {const char*} 要打开的数据字段名集合，格式为
	 *  由分隔符 ",; \t" 分隔的字段名称，如：user_id,user_name,user_mail
	 * @return {bool} 是否成功
	 */
	static bool build_open(string& out, int id,
		const char* dbn, const char* tbl,
		const char* idx, const char* flds);

	/**
	 * 创建查询数据库记录的请求协议数据
	 * @param out {string&} 存储请求协议结果
	 * @param id {int} 对应打开索引的表ID号
	 * @param values {const char*[]} 匹配字段值数组，字段值的加入顺序应与打开索引 
	 *  中各个字段的顺序相同
	 * @param num {int} values 数组长度，该值不应超过在打开索引时的字段个数
	 * @param cond {const char*} 匹配条件，可以为：
	 *  = 等于; >= 大于等于; > 大于; < 小于; <= 小于等于
	 * @param nlimit {int} 结果集个数限制，0 表示不限制个数
	 * @param noffset {int} 结果集开始位置(0表示从第一个结果开始)
	 * @return {bool} 是否成功
	 */
	static bool build_get(string& out, int id,
		const char* values[], int num,
		const char* cond = "=", int nlimit = 0, int noffset = 0);

	/**
	 * 创建查询数据库记录的请求协议数据
	 * @param out {string&} 存储请求协议数据
	 * @param id {int} 对应打开索引的表ID号
	 * @param nfld {int} 在打开索引时字段个数
	 * @param first_value {const char*} 第一个参数
	 * @param ... {const char*} 参数列表，最后一个参数为 NULL 表示结束
	 * @return {bool} 是否成功
	 */
	static bool ACL_CPP_PRINTF(4, 5) build_get(string& out, int id,
		int nfld, const char* first_value, ...);

	/**
	 * 创建修改数据库记录的请求协议数据
	 * @param out {string&} 存储请求协议数据
	 * @param id {int} 对应打开索引的表ID号
	 * @param values {const char*[]} 匹配字段值数组，字段值的加入顺序应与打开索引 
	 *  中各个字段的顺序相同
	 * @param num {int} values 数组长度，该值不应超过在打开索引时的字段个数
	 * @param to_values {cosnt *[]} 匹配字段新值，字段值的顺序应与 open 方法中
	 *  的字段顺序相同
	 * @param to_num {int} to_values 数组长度
	 * @param cond {const char*} 匹配条件，可以为：
	 * @param nlimit {int} 结果集个数限制，0 表示不限制个数
	 * @param noffset {int} 结果集开始位置(0表示从第一个结果开始)
	 * @return {bool} 是否成功
	 */
	static bool build_mod(string& out, int id,
		const char* values[], int num,
		const char* to_values[], int to_num,
		const char* cond = "=", int nlimit = 0, int noffset = 0);

	/**
	 * 创建删除数据库记录的请求协议数据
	 * @param out {string&} 存储请求协议数据
	 * @param id {int} 对应打开索引的表ID号
	 * @param values {const char*[]} 匹配字段值数组，字段值的加入顺序应与打开索引 
	 *  中各个字段的顺序相同
	 * @param num {int} values 数组长度，该值不应超过在打开索引时的字段个数
	 * @param cond {const char*} 匹配条件，可以为：
	 * @param nlimit {int} 结果集个数限制，0 表示不限制个数
	 * @param noffset {int} 结果集开始位置(0表示从第一个结果开始)
	 * @return {bool} 是否成功
	 */
	static bool build_del(string& out, int id, const char* values[],
		int num, const char* cond = "=",
		int nlimit = 0, int noffset = 0);

	/**
	 * 创建删除数据库记录的请求协议数据
	 * @param out {string&} 存储请求协议数据
	 * @param id {int} 对应打开索引的表ID号
	 * @param nfld {int} 在打开索引时字段个数
	 * @param first_value {const char*} 第一个参数
	 * @param ... {const char*} 参数列表，最后一个参数为 NULL 表示结束
	 * @return {bool} 是否成功
	 */
	static bool ACL_CPP_PRINTF(4, 5) build_del(string& out, int id,
		int nfld, const char* first_value, ...);

	/**
	 * 创建添加数据库记录的请求协议数据
	 * @param out {string&} 存储请求协议数据
	 * @param id {int} 对应打开索引的表ID号
	 * @param values {const char*[]} 匹配字段值数组，字段值的加入顺序应与打开索引 
	 *  中各个字段的顺序相同
	 * @param num {int} values 数组长度，该值不应超过在打开索引时的字段个数
	 * @return {bool} 是否成功
	 */
	static bool build_add(string& out, int id,
		const char* values[], int num);

	/**
	 * 创建添加数据库记录的请求协议数据
	 * @param out {string&} 存储请求协议数据
	 * @param id {int} 对应打开索引的表ID号
	 * @param nfld {int} 在打开索引时字段个数
	 * @param first_value {const char*} 第一个参数
	 * @param ... {const char*} 参数列表，最后一个参数为 NULL 表示结束
	 * @return {bool} 是否成功
	 */
	static bool ACL_CPP_PRINTF(4, 5) build_add(string& out, int id,
		int nfld, const char* first_value, ...);

	/**
	 * 通用的创建数据库处理的请求协议数据
	 * @param out {string&} 存储请求协议数据
	 * @param id {int} 对应打开索引的表ID号
	 * @param oper {const char*} 操作方式，对应的操作符为：
	 *  添加: +
	 *  查询: =, >, >=, <, <=
	 *  修改: =, >, >=, <, <=
	 *  删除: =, >, >=, <, <=
	 * @param values {const char*[]} 匹配字段值数组，字段值的加入顺序应与打开索引 
	 *  中各个字段的顺序相同
	 * @param num {int} values 数组长度，该值不应超过在打开索引时的字段个数
	 * @param limit_offset {const char*} 要求的查询范围
	 * @param mop {char} 仅针对删除，修改操作有效，其对应的操作符分别为:
	 *  D: 删除, U: 修改
	 * @param to_values {const char*[]} 目标值指针数组
	 * @param to_num {int} to_values 数组的长度
	 */
	static void build_request(string& out, int id, const char* oper,
		const char* values[], int num,
		const char* limit_offset, char mop,
		const char* to_values[], int to_num);

	/**
	 * 分析数据库的返回数据
	 * @param nfld {int} 打开的表的元素个数
	 * @param in {string&} 从数据库读到的数据行, 尾部应该不包含 "\r\n"
	 * @param errnum_out {int&} 存储处理过程的出错号，参见: hserror.hpp
	 * @param serror_out {const char*&} 存储处理过程的出错描述信息
	 * @return {bool} 分析是否成功
	 */
	bool parse_respond(int nfld, string& in,
                int& errnum_out, const char*& serror_out);

	/**
	 * 当执行查询语句时，可以通过此函数获得查询的结果集
	 * @return {const std::vector<hsrow*>&}
	 */
	const std::vector<hsrow*>& get();

	/**
	 * 当用户用完查询结果集后进行第二次查询时调用此函数清理上次查询结果
	 */
	void reset();
private:
	bool  debugOn_;
	bool  cache_enable_;
	//int   nfld_;
	int   ntoken_;
	char* buf_ptr_;

	// 查询结果集
	std::vector<hsrow*> rows_;

	// 行记录对象缓存，当行记录对象用完后，为了保证内存
	// 复用，将不再使用的行记录对象进行缓存，以备后用
	std::vector<hsrow*> rows_cache_;

	// 清除行缓存对象集合
	void clear_cache();

	// 获得下一个查询结果
	hsrow* get_next_row();
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY
