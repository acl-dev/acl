#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../stream/socket_stream.hpp"
#include "../hsocket/hsproto.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_ARGV;

namespace acl {

class hsrow;
class hstable;

class ACL_CPP_API hsclient : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param addr {const char*} handlersocket 插件在 Mysql 上的监听地址，
	 * @param cache_enable {bool} 内部是否启用行缓存功能
	 * @param retry_enable {bool} 当因为网络原因出错时是否需要重试
	 *  格式为：ip:port
	 */
	hsclient(const char* addr, bool cache_enable = true, bool retry_enable = true);
	~hsclient();

	/**
	 * 查询与所给字段值匹配的结果
	 * @param values {const char*[]} 匹配字段值数组，字段值的加入顺序应与 open
	 *  函数中 flds 中各个字段的顺序相同
	 * @param num {int} values 数组长度，该值不应超过构造函数中 flds 所含有的
	 *  字段个数
	 * @param cond {const char*} 匹配条件，可以为：
	 *  = 等于; >= 大于等于; > 大于; < 小于; <= 小于等于
	 * @param nlimit {int} 结果集个数限制，0 表示不限制个数
	 * @param noffset {int} 结果集开始位置(0表示从第一个结果开始)
	 * @return {const std::verctor<hsrow*>&} 返回结果集
	 */
	const std::vector<hsrow*>& get(const char* values[], int num,
		const char* cond = "=", int nlimit = 0, int noffset = 0);

	/**
	 * 查询与所给字段值匹配的结果
	 * @param first_value {const char*} 为对应于构造函数中 flds 字段集中
	 *  的第一个字段的字段值
	 * @param ... {const char*} 参数列表，最后一个参数为 NULL 表示结束
	 * @return {const std::verctor<hsrow*>&} 返回结果集
	 */
	const std::vector<hsrow*>& get(const char* first_value, ...)
		ACL_CPP_PRINTF(2, 3);

	/**
	 * 更新数据库表中匹配字段的数值
	 * @param values {const char*[]} 匹配字段值数组，字段值的加入顺序应与 open
	 *  函数中 flds 中各个字段的顺序相同
	 * @param num {int} values 数组长度，该值不应超过构造函数中 flds 所含有的
	 *  字段个数
	 * @param to_values {cosnt *[]} 匹配字段新值，字段值的顺序应与 open 方法中
	 *  的字段顺序相同
	 * @param to_num {int} to_values 数组长度
	 * @param cond {const char*} 匹配条件，可以为：
	 *  = 等于; >= 大于等于; > 大于; < 小于; <= 小于等于
	 * @param nlimit {int} 结果集个数限制，0 表示不限制个数
	 * @param noffset {int} 结果集开始位置(0表示从第一个结果开始)
	 * @return {bool} 更新是否成功
	 */
	bool mod(const char* values[], int num,
		const char* to_values[], int to_num,
		const char* cond = "=", int nlimit = 0, int noffset = 0);

	/**
	 * 删除数据库表中匹配字段的记录
	 * @param values {const char*[]} 匹配字段值数组，字段值的加入顺序应与 open
	 *  函数中 flds 中各个字段的顺序相同
	 * @param num {int} values 数组长度，该值不应超过构造函数中 flds 所含有的
	 *  字段个数
	 * @param cond {const char*} 匹配条件，可以为：
	 *  = 等于; >= 大于等于; > 大于; < 小于; <= 小于等于
	 * @param nlimit {int} 删除数据的个数, 0 表示不限制
	 * @param noffset {int} 结果集开始位置(0表示从第一个结果开始)
	 * @return {bool} 更新是否成功
	 */
	bool del(const char* values[], int num, const char* cond = "=",
		int nlimit = 0, int noffset = 0);

	/**
	 * 删除数据库表中匹配字段的记录
	 * @param first_value {const char*} 为对应于构造函数中 flds 字段集中
	 *  的第一个字段的字段值
	 * @param ... {const char*} 参数列表，最后一个参数为 NULL 表示结束
	 * @return {bool} 添加记录是否成功
	 */
	bool fmt_del(const char* first_value, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * 向数据库添加新记录
	 * @param values {const char*[]} 匹配字段值数组，字段值的加入顺序应与构造
	 *  函数中 flds 中各个字段的顺序相同
	 * @param num {int} values 数组长度，该值不应超过构造函数中 flds 所含有的
	 *  字段个数
	 * @return {bool} 添加记录是否成功
	 */
	bool add(const char* values[], int num);

	/**
	 * 向数据库中添加新记录
	 * @param first_value {const char*} 为对应于构造函数中 flds 字段集中
	 *  的第一个字段的字段值
	 * @param ... {const char*} 参数列表，最后一个参数为 NULL 表示结束
	 * @return {bool} 添加记录是否成功
	 */
	bool fmt_add(const char* first_value, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * 设置是否进行调试
	 * @param on {bool} true 则表示进行调试，会将一些中间信息记入日志中
	 */
	void debug_enable(bool on);

	/**
	 * 打开数据库表
	 * @param dbn {const char*} 数据库名称
	 * @param tbl {const char*} 数据库表名
	 * @param idx {const char*} 索引字段名
	 * @param flds {const char*} 要打开的数据字段名集合，格式为
	 *  由分隔符 ",; \t" 分隔的字段名称，如：user_id,user_name,user_mail
	 * @param auto_open {bool} 当表未打开是否自动打开
	 * @return {bool} true 表示正常打开，否则表示打开表失败
	 */
	bool open_tbl(const char* dbn, const char* tbl,
		const char* idx, const char* flds, bool auto_open = true);

	/**
	 * 获得连接地址
	 * @return {const char*} 永不为空
	 */
	const char* get_addr() const;

	/**
	 * 获得出错错误号
	 * @return {int}
	 */
	int  get_error() const;

	/**
	 * 获得出错错误信息描述
	 * @param errnum {int} 由 get_error 获得的错误号
	 * @return {const char*}
	 */
	const char* get_serror(int errnum) const;

	/**
	 * 获得上次出错时的错误描述信息
	 * @return {const char*}
	 */
	const char* get_last_serror() const;

	/**
	 * 获得当前 hsclient 对象所用的 id 号
	 * @return {int}
	 */
	int get_id() const;
private:
	bool   debugOn_;
	char*  addr_;
	hsproto  proto_;
	bool   retry_enable_;
	int    id_max_;
	hstable* tbl_curr_;
	string buf_;

	// 服务器连接流
	socket_stream stream_;
	std::map<string, hstable*> tables_;

	char   cond_def_[2];
	int    error_;
	const char* serror_;

	// 打开数据库连接
	bool open_tbl(const char* dbn, const char* tbl,
		const char* idx, const char* flds, const char* key);

	// 当读写数据库连接流出错时需要调用此函数来关闭连接流及释放
	// 表对象
	void close_stream();

	// 清理内部打开的数据库表的对象
	void clear_tables();

	// 向数据库发送查询命令并取得结果数据
	bool query(const char* oper, const char* values[], int num,
		const char* limit_offset, char mop,
		const char* to_values[], int to_num);
	bool chat();
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
