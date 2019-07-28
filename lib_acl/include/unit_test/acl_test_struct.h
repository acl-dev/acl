#ifndef	ACL_TEST_STRUCT_INCLUDE_H
#define	ACL_TEST_STRUCT_INCLUDE_H

# ifdef	__cplusplus
extern "C" {
# endif

#include "../stdlib/acl_array.h"

typedef struct AUT_LINE {
	char  cmd_name[128];	/* 命令函数名称 */
	int   result;		/* 执行结果 */
	int   argc;		/* 参数个数 */
	ACL_ARRAY *argv;	/* 参数列表 */
	char *args_str;		/* 参数列表的备份 */
	int   valid_line_idx;	/* 该有效配置行在所有有效配置行中的行号 */
	int   line_number;	/* 该有效配置行在配置文件中的行号 */
	void *arg_inner;	/* 内部注册自己的参数到该结构中 */
	void (*free_arg_inner)(void*);
	void *arg_outer;	/* 外部应用注册自己的参数到该结构中 */
	int   obj_type;		/* 是内部命令对象还是外部命令对象标志位,
				 * defined as: AUT_OBJ_
				 */
} AUT_LINE;

typedef int (*AUT_FN) (AUT_LINE *test_line, void *arg);

/**
 * 说明: 单元测试所采用的一致的数据结构
 */
typedef struct AUT_FN_ITEM {
	const char *cmd_name;		/* 命令字名称 */
	const char *fn_name;		/* 函数名称 */
	AUT_FN fn_callback;		/* 测试用回调函数 */
	void *arg;			/* 测试回调函数所用的参数 */
	int   inner;			/* 是否是内部的命令 */
} AUT_FN_ITEM;

/* 内部数据结构定义 */
typedef struct {
	char *name;
	char *value;
} AUT_ARG_ITEM;

typedef struct {
	int match_number;		/* 成对命令中相互间的匹配号,
					 * 例如: loop_begin 与 loop_end 之间
					 * 对非成对的命令项无效, 如对:
					 * stop 命令项.
					 */
	AUT_LINE *peer;			/* 与该有效配置行成对的另一个对象 */
	int flag;			/* define as: AUT_FLAG_ */
	int status;			/* define as: AUT_STAT_ */
	int valid_line_idx;		/* 在所有有效配置行中的下标位置 */

	/* 私有类型定义如下 */
	/* 针对循环执行命令序列 */
	int nloop_max;			/* 最大循环次数, 由配置文件中获得 */
	int nloop_cur;			/* 当前循环的次数 */
	int offset_valid_line_idx;	/* 相对有效配置行下标索引 */
	int loop_sleep;			/* 循环执行时的休息 */
} AUT_CMD_TOKEN;

#define	AUT_OBJ_OUTER		0	/* 默认为外部命令对象 */
#define	AUT_OBJ_INNER		1	/* 为内部对象 */

#define	AUT_FLAG_LOOP_BEGIN	1
#define	AUT_FLAG_LOOP_BREAK	2
#define	AUT_FLAG_LOOP_CONTINUE	3
#define	AUT_FLAG_LOOP_END	4

#define	AUT_FLAG_IF		5
#define	AUT_FLAG_ELSE		6
#define	AUT_FLAG_ELIF		7
#define	AUT_FLAG_ENDIF		8

#define	AUT_FLAG_STOP		9

#define	AUT_STAT_FREE		0
#define	AUT_STAT_BUSY		1


#define	AUT_LOOP_BREAK			-100

#define	VAR_AUT_LOG_PRINT	0x0001
#define	VAR_AUT_LOG_FPRINT	0x0010

/* 配置文件中的关键字 */
/* 日志记录级别 */
#define	VAR_AUT_LOG			"LOG"

/* 执行停止标志位 */
#define	VAR_AUT_STOP			"STOP"

/* 休息标志位 */
#define	VAR_AUT_SLEEP			"SLEEP"

/* 暂停标志位 */
#define	VAR_AUT_PAUSE			"PAUSE"

/* 循环执行开始标志位 */
#define	VAR_AUT_LOOP_BEGIN		"LOOP_BEGIN"

/* 循环执行停止标志位 */
#define	VAR_AUT_LOOP_END		"LOOP_END"

/* 跳出循环执行 */
#define	VAR_AUT_LOOP_BREAK		"LOOP_BREAK"

/* 循环继续 */
#define	VAR_AUT_LOOP_CONTINUE		"LOOP_CONTINUE"

/* 条件判断开始语句 */
#define	VAR_AUT_IF			"IF"

/* 条件判断 else 语句 */
#define	VAR_AUT_ELSE			"ELSE"

/* 条件判断结束语句 */
#define	VAR_AUT_ENDIF			"ENDIF"

/* 跳转执行语句 */
#define	VAR_AUT_GOTO			"GOTO"

/*----------------- 内部保留的一些配置文件参数 -----------------------------*/
/**
 * 通用的整数值参数名变量:
 *
 * 对于 VAR_AUT_SLEEP 则表示为休息的秒数值;
 * 对于 VAR_AUT_LOOP_BEGIN 则表示循环的次数
 */

#define	VAR_AUT_ITEM_COUNT		"COUNT"


# ifdef	__cplusplus
}
# endif

#endif

