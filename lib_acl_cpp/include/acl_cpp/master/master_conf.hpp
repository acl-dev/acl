#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_XINETD_CFG_PARSER;
struct ACL_CFG_INT_TABLE;
struct ACL_CFG_INT64_TABLE;
struct ACL_CFG_STR_TABLE;
struct ACL_CFG_BOOL_TABLE;

namespace acl {

typedef struct master_int_tbl
{
	const char *name;
	int  defval;
	int *target;
	int  min;
	int  max;
} master_int_tbl;

typedef struct master_str_tbl
{
	const char *name;
	const char *defval;
	char **target;
} master_str_tbl;

typedef struct master_bool_tbl
{
	const char *name;
	int   defval;
	int  *target;
} master_bool_tbl;

typedef struct master_int64_tbl
{
	const char *name;
#if defined(_WIN32) || defined(_WIN64)
	__int64  defval;
	__int64 *target;
	__int64  min;
	__int64  max;
#else
	long long int  defval;
	long long int *target;
	long long int  min;
	long long int  max;
#endif
} master_int64_tbl;

class master_base;

class ACL_CPP_API master_conf : public noncopyable
{
public:
	master_conf();
	~master_conf();

	/**
	 * 设置 bool 类型的配置项
	 * @param table {master_bool_tbl*}
	 */
	void set_cfg_bool(master_bool_tbl* table);

	/**
	 * 设置 int 类型的配置项
	 * @param table {master_int_tbl*}
	 */
	void set_cfg_int(master_int_tbl* table);

	/**
	 * 设置 int64 类型的配置项
	 * @param table {master_int64_tbl*}
	 */
	void set_cfg_int64(master_int64_tbl* table);

	/**
	 * 设置 字符串 类型的配置项
	 * @param table {master_str_tbl*}
	 */
	void set_cfg_str(master_str_tbl* table);

	/**
	 * 加载配置文件
	 * @param path {const char*} 配置文件全路径
	 */
	void load(const char* path);

	/**
	 * 获得由 load 设置的配置文件路径
	 * @return {const char*} 返回 NULL 表示没有设置配置文件路径
	 */
	const char* get_path(void) const;

	/**
	 * 重置配置解析器状态，释放之前分配的资源，调用此函数后，
	 * 之前获得的字符串配置项的内存将会被释放，所以禁止再用；
	 * 调用该函数后，则该配置解析器对象可以再次使用解析其它
	 * 配置文件
	 */
	void reset(void);

	ACL_CFG_INT_TABLE* get_int_cfg() const;
	ACL_CFG_INT64_TABLE* get_int64_cfg() const;
	ACL_CFG_STR_TABLE* get_str_cfg() const;
	ACL_CFG_BOOL_TABLE* get_bool_cfg() const;

private:
	char* path_;
	bool  cfg_loaded_;

	ACL_XINETD_CFG_PARSER* cfg_;
	ACL_CFG_INT_TABLE*  int_cfg_;
	ACL_CFG_INT64_TABLE* int64_cfg_;
	ACL_CFG_STR_TABLE*  str_cfg_;
	ACL_CFG_BOOL_TABLE* bool_cfg_;

	master_int_tbl* int_tbl_;
	master_str_tbl* str_tbl_;
	master_bool_tbl* bool_tbl_;
	master_int64_tbl* int64_tbl_;

	void load_str(void);
	void load_bool(void);
	void load_int(void);
	void load_int64(void);
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
