#ifndef	ACL_TEST_STRUCT_INCLUDE_H
#define	ACL_TEST_STRUCT_INCLUDE_H

# ifdef	__cplusplus
extern "C" {
# endif

#include "../stdlib/acl_array.h"

typedef struct AUT_LINE {
	char  cmd_name[128];	/* command function name */
	int   result;		/* execution result */
	int   argc;		/* parameter count */
	ACL_ARRAY *argv;	/* parameter list */
	char *args_str;		/* parameter list text string */
	int   valid_line_idx;	/* index position of this valid test
				 * command in the valid command array */
	int   line_number;	/* line number of this valid test
				 * command in the configuration file */
	void *arg_inner;	/* internal registered user's own
				 * parameter structure pointer */
	void (*free_arg_inner)(void*);
	void *arg_outer;	/* external application registered
				 * user's own parameter structure pointer */
	int   obj_type;		/* flag bit for internal command or external command,
				 * defined as: AUT_OBJ_
				 */
} AUT_LINE;

typedef int (*AUT_FN) (AUT_LINE *test_line, void *arg);

/**
 * Description: Data structure used by unit test functions.
 */
typedef struct AUT_FN_ITEM {
	const char *cmd_name;		/* command function name */
	const char *fn_name;		/* function name */
	AUT_FN fn_callback;		/* test callback function */
	void *arg;			/* parameter passed to test callback function */
	int   inner;			/* whether it is an internal command */
} AUT_FN_ITEM;

/* Internal data structure definition */
typedef struct {
	char *name;
	char *value;
} AUT_ARG_ITEM;

typedef struct {
	int match_number;		/* number of mutually matched pairs,
					 * Note: between loop_begin and loop_end
					 * effective for non-paired commands, otherwise:
					 * stop command.
					 */
	AUT_LINE *peer;			/* another structure paired with this valid test command */
	int flag;			/* define as: AUT_FLAG_ */
	int status;			/* define as: AUT_STAT_ */
	int valid_line_idx;		/* index position in the valid command array */

	/* private loop execution data */
	/* loop execution related data */
	int nloop_max;			/* maximum loop count, read from configuration file */
	int nloop_cur;			/* current loop iteration */
	int offset_valid_line_idx;	/* offset in valid command array index */
	int loop_sleep;			/* sleep time during loop execution */
} AUT_CMD_TOKEN;

#define	AUT_OBJ_OUTER		0	/* default is external command */
#define	AUT_OBJ_INNER		1	/* is internal command */

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

/* Keywords in configuration file */
/* Log record flag */
#define	VAR_AUT_LOG			"LOG"

/* Execution stop flag bit */
#define	VAR_AUT_STOP			"STOP"

/* Sleep flag bit */
#define	VAR_AUT_SLEEP			"SLEEP"

/* Pause flag bit */
#define	VAR_AUT_PAUSE			"PAUSE"

/* Loop execution start flag bit */
#define	VAR_AUT_LOOP_BEGIN		"LOOP_BEGIN"

/* Loop execution stop flag bit */
#define	VAR_AUT_LOOP_END		"LOOP_END"

/* Break loop execution */
#define	VAR_AUT_LOOP_BREAK		"LOOP_BREAK"

/* Loop continue */
#define	VAR_AUT_LOOP_CONTINUE		"LOOP_CONTINUE"

/* Conditional judgment start flag */
#define	VAR_AUT_IF			"IF"

/* Conditional judgment else flag */
#define	VAR_AUT_ELSE			"ELSE"

/* Conditional judgment end flag */
#define	VAR_AUT_ENDIF			"ENDIF"

/* Jump execution flag */
#define	VAR_AUT_GOTO			"GOTO"

/*----------------- Internal use of some configuration file
 * keywords -----------------------------*/
/**
 * Common parameter value description:
 *
 * For VAR_AUT_SLEEP, it represents the sleep time value;
 * For VAR_AUT_LOOP_BEGIN, it represents the loop count
 */

#define	VAR_AUT_ITEM_COUNT		"COUNT"


# ifdef	__cplusplus
}
# endif

#endif

