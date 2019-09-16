#ifndef	ACL_TEST_GLOBAL_INCLUDE_H
#define	ACL_TEST_GLOBAL_INCLUDE_H

# ifdef	__plusplus
extern "C" {
# endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_loadcfg.h"
#include "../stdlib/acl_array.h"

#include "acl_test_struct.h"

/*-----------------------  内部函数接口  -----------------------------------*/
/* in acl_test_cfg.c */
ACL_API ACL_ARRAY *aut_parse_args_list(const char *str_in);
ACL_API void aut_free_args_list(ACL_ARRAY *a);

/* in acl_test_cfg_general.c */
ACL_API int aut_cfg_add_general_line(const ACL_CFG_LINE *line);

/* in acl_test_inner.c*/
ACL_API AUT_LINE *aut_add_inner_cmd(const ACL_CFG_LINE *line);

/* in acl_test_outer.c */
ACL_API AUT_LINE *aut_add_outer_cmd(const ACL_CFG_LINE *line);

/* in acl_test_token.c */
ACL_API AUT_CMD_TOKEN *aut_line_peer_token(const AUT_LINE *test_line);
ACL_API AUT_LINE *aut_line_peer(const AUT_LINE *test_line);

/* in acl_test_misc.c */
ACL_API void aut_line_free(void *ctx);

/*------------------ 外部可以调用的一些函数接口 ----------------------------*/

/* in acl_test_cfg.c */

/**
 * 功能:  读取配置文件
 * @param  pathname 配置文件名
 * @return  0 表示成功, -1 表示失败
 */
ACL_API int aut_cfg_parse(const char *pathname);

/**
 * 功能: 打印所有有效的配置行内容
 * @return 成功: 0, 失败: -1
 */
ACL_API int aut_cfg_print(void);

/* in acl_test_misc.c */

ACL_API AUT_LINE *aut_line_new(const ACL_CFG_LINE *cfg_line);

/**
 * 功能: 获得对应于命令字的配置参数集合的容器, 以动态数组表示
 * @param cmd_name 命令字
 * @return 成功: 非空指针, 失败: 空指针
 */
ACL_API const ACL_ARRAY *aut_args_get(const char *cmd_name);

/**
 * 功能: 获得有效配置项的总条目
 * @return 成功: >0, 失败: -1
 */
ACL_API int aut_size(void);

/**
 * 功能: 根据下标值取出所对应的有效配置行结构指针
 * @param idx 下标索引值
 * @return 成功: 非空结构指针, 失败: 空指针
 *
 */
ACL_API AUT_LINE *aut_index(int idx);

/**
 * 功能: 比较所给的命令是否与  test_line 中所记录的命令相同
 * @param test_line 与某一有效配置行相关的结构指针
 * @param cmd_name 待查询的命令
 * @return 如果相等则返回 0, 如果不等则返回非 0
 */
ACL_API int aut_line_cmdcmp(const AUT_LINE *test_line, const char *cmd_name);

/**
 * 功能: 比较所给的执行结果值是否与配置文件中所期望的结果值相等
 * @param test_line 与某一有效配置行相关的结构指针
 * @param result 程序某个任务函数的执行结果值
 * @return 相等则返回 0, 不相等则返回非 0
 */
ACL_API int aut_line_resultcmp(const AUT_LINE *test_line, int result);

/**
 * 功能: 取得该有效配置行在配置文件中的行号位置
 * @param test_line 与某一有效配置行相关的结构指针
 * @return 成功: >=0, 该值即为行号, 失败: < 0
 */
ACL_API int aut_line_number(const AUT_LINE *test_line);

/**
 * 功能: 取得所给命令行的有效行号
 * @param test_line 与某一有效配置行相关的结构指针
 * @return 成功: >=0, 该值即为行号, 失败: < 0
 */
ACL_API int aut_line_valid_linenum(const AUT_LINE *test_line);

/**
 * 功能: 获得该配置行的命令字
 * @param test_line 与某一有效配置行相关的结构指针
 * @return 相等返回 0, 不等则返回非 0
 */
ACL_API const char *aut_line_cmdname(const AUT_LINE *test_line);

/**
 * 功能: 返回该配置行中参数的个数
 * @param test_line 与某一有效配置行相关的结构指针
 * @return 成功: >= 0, 失败: -1
 */
ACL_API int aut_line_argc(const AUT_LINE *test_line);

/**
 * 功能: 读取配置文件行中的参数值
 * @param  test_line AUT_LINE 结构指针
 * @param  name 要查找的关建字
 * @return 成功: 非空指针, 失败: 空指针
 */
ACL_API const char *aut_line_getvalue(const AUT_LINE *test_line, const char *name);

/**
 * 功能: 返回该配置行的内容
 * @param test_line 与某一有效配置行相关的结构指针
 * @return 成功: 非空指针, 失败: 空指针
 */
ACL_API const char *aut_line_argstr(const AUT_LINE *test_line);

/**
 * 功能: 返回该配置行中期望的执行结果值
 * @param test_line 与某一有效配置行相关的结构指针
 * @return 期望的执行结果值
 * 说明: 没有出错的情况, 如果传入的参数非法则返回 -1, 但无法区分该 -1 值是
 *       非法值还是期望的执行结果值
 */
ACL_API int aut_line_result(const AUT_LINE *test_line);

/**
 * 功能: 是否遇到了配置行中的结束字段
 * @param test_line 与某一有效配置行相关的结构指针
 * @return 是: 1, 否: 0
 */
ACL_API int aut_line_stop(const AUT_LINE *test_line);

/**
 * 功能: 内部保留字段, 遇到此内部保留配置行则需要跳过
 * @param test_line 与某一有效配置行相关的结构指针
 * @return 是: 1, 否: 0
 */
ACL_API int aut_line_reserved(AUT_LINE *test_line);

/**
 * 功能: 调用者可以把自己的参数添加到 test_line 之中
 * @param test_line 与某一有效配置行相关的结构指针
 * @param arg 用户要添加的参数
 * @return 成功: 0, 失败: -1
 */
ACL_API int aut_line_add_arg(AUT_LINE *test_line, void *arg);

/**
 * 功能: 从 test_line 删除用户自己的参数
 * @param test_line 与某一有效配置行相关的结构指针
 * @param free_fn 用户自己的析构函数
 */
ACL_API void aut_line_del_arg(AUT_LINE *test_line, void (*free_fn) (void *));

/**
 * 功能: 从 test_line 中取出用户自己的参数
 * @param test_line 与某一有效配置行相关的结构指针
 * @return 成功: 非空指针, 如果返回指针为空则有可能是内部错误或本来就是空
 */
ACL_API void *aut_line_get_arg(const AUT_LINE *test_line);

/**
 * 功能: 取得所给命令行的结尾行
 * @param start_linenum 命令开始执行点所在行号
 * @return >= 0 ok; < 0 未找到
 */
ACL_API int aut_end_linenum(int start_linenum);

/**
 * 功能: 从当前所提供的 test_line 起, 一直向下找到某个与所提供标志位相同的
 *       test_line.
 * @param test_line 与某一有效配置行相关的结构指针
 * @param flag defined as AUT_FLAG_ in acl_test_struct.h
 * @return != NULL, ok find it; == NULL, not found.
 */
ACL_API const AUT_LINE *aut_lookup_from_line(const AUT_LINE *test_line, int flag);

/* in acl_test_runner.c */

/**
 * 功能 执行所有注册的测试函数, 如果有任何一个任务执行的结果与预期结果不一致则退
 *      出执行
 * @return 成功: 0, 失败: -1
 */
ACL_API int aut_start(void);

/**
 * 测试过程结束后需要调用此函数以释放一些内存资源
 */
ACL_API void aut_stop(void);

/**
 * 功能  将需要进行单元测试的任务函数注册
 * @param test_fn_tab 单元测试函数结构数组
 */
ACL_API void aut_register(const AUT_FN_ITEM test_fn_tab[]);

/* in acl_test_loop.c */
ACL_API AUT_LINE *aut_loop_make_begin(const ACL_CFG_LINE *cfg_line);
ACL_API AUT_LINE *aut_loop_make_break(const ACL_CFG_LINE *cfg_line);
ACL_API AUT_LINE *aut_loop_make_end(const ACL_CFG_LINE *cfg_line);
ACL_API const AUT_LINE *aut_loop_end(const AUT_LINE *test_begin);
ACL_API int aut_loop_count(const AUT_LINE *test_line);
ACL_API int aut_loop_from(const AUT_LINE *test_line);
ACL_API int aut_loop_to(const AUT_LINE *test_line);

/* in acl_test_log.c */

ACL_API int  aut_log_open(const char *pathname);
ACL_API void aut_log_info(const char *format, ...);
ACL_API void aut_log_warn(const char *format, ...);
ACL_API void aut_log_error(const char *format, ...);
ACL_API void aut_log_fatal(const char *format, ...);
ACL_API void aut_log_panic(const char *format, ...);

# ifdef	__plusplus
}
# endif

#endif

