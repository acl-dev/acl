#ifndef	ACL_XINETD_CFG_INCLUDE_H
#define	ACL_XINETD_CFG_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_array.h"

/**
 * 配置文件解析句柄类型定义
 */
typedef struct ACL_XINETD_CFG_PARSER ACL_XINETD_CFG_PARSER;

/**
 * 功能: 获得所需要的配置 项的内容
 * @param xcp 结构指针, 不能为空
 * @param name 配置项的变量名
 * @return 配置文件中配置项的内容
 */
ACL_API const char *acl_xinetd_cfg_get(const ACL_XINETD_CFG_PARSER *xcp, const char *name);

/**
 * 功能: 获得所需要的配置项的数组，对于一个变量名对应多个变量值时有用
 * @param xcp 结构指针, 不能为空
 * @param name 配置项的变量名
 * @return 配置文件中配置项的内容动态数组
 */
ACL_API const ACL_ARRAY *acl_xinetd_cfg_get_ex(const ACL_XINETD_CFG_PARSER *xcp, const char *name);

/**
 * 功能: 从配置文件中获得对应在于某一个索引值位置的内容
 * @param xcp 结构指针, 不能为空
 * @param idx 索引位置值
 * @param ppname 指向指针的地址的变量
 * @param ppvalue 指向指针的地址的变量
 * @return  0: OK, -1: ERR
 */
ACL_API int acl_xinetd_cfg_index(const ACL_XINETD_CFG_PARSER *xcp,
			int idx,
			char **ppname,
			char **ppvalue);

/**
 * 功能: 配置文件中配置项的条数
 * @param xcp 结构指针, 不能为空
 * @return 配置文件中配置项的条数
 */
ACL_API int acl_xinetd_cfg_size(const ACL_XINETD_CFG_PARSER *xcp);

/**
 * 功能: 释放由结构指针所指向的内存空间
 * @param xcp 结构指针
 */
ACL_API void acl_xinetd_cfg_free(ACL_XINETD_CFG_PARSER *xcp);

/**
 * 功能: 读取配置文件并进行解析
 * @param pathname 配置文件的文件名
 * @return 已经解析了配置文件的结构指针
 */
ACL_API ACL_XINETD_CFG_PARSER *acl_xinetd_cfg_load(const char *pathname);

/**
 * 整数类配置项结构
 */
typedef struct ACL_CFG_INT_TABLE {
	const char *name;
	int  defval;
	int *target;
	int  min;
	int  max;
} ACL_CFG_INT_TABLE;

/**
 * 64 位整数类配置项结构
 */
typedef struct ACL_CFG_INT64_TABLE {
	const char *name;
	acl_int64  defval;
	acl_int64 *target;
	acl_int64  min;
	acl_int64  max;
} ACL_CFG_INT64_TABLE;

/**
 * 字符串类配置项结构
 */
typedef struct ACL_CFG_STR_TABLE {
	const char *name;
	const char *defval;
	char **target;
} ACL_CFG_STR_TABLE;

/**
 * 布尔型类配置项结构
 */
typedef struct ACL_CFG_BOOL_TABLE {
	const char *name;
	int   defval;
	int  *target;
} ACL_CFG_BOOL_TABLE;

/* in acl_xinetd_params.c */

/**
 * 从配置文件解析器中读取整数类型的表
 * @param cfg {ACL_XINETD_CFG_PARSER*} 当为空时则用默认值进行赋值
 * @param table {ACL_CFG_INT_TABLE*}
 */
ACL_API void acl_xinetd_params_int_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_INT_TABLE *table);

/**
 * 从配置文件解析器中读取 64 位整数类型的表
 * @param cfg {ACL_XINETD_CFG_PARSER*} 当为空时则用默认值进行赋值
 * @param table {ACL_CFG_INT64_TABLE*}
 */
ACL_API void acl_xinetd_params_int64_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_INT64_TABLE *table);

/**
* 从配置文件解析器中读取字符串类型的表
* @param cfg {ACL_XINETD_CFG_PARSER*} 当为空时则用默认值进行赋值
* @param table {ACL_CFG_STR_TABLE*}
*/
ACL_API void  acl_xinetd_params_str_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_STR_TABLE *table);

/**
* 从配置文件解析器中读取BOOL类型的表
* @param cfg {ACL_XINETD_CFG_PARSER*} 当为空时则用默认值进行赋值
* @param table {ACL_CFG_BOOL_TABLE*}
*/
ACL_API void  acl_xinetd_params_bool_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_BOOL_TABLE *table);

#ifdef	__cplusplus
}
#endif

#endif

