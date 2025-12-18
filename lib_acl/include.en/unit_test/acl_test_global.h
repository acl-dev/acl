#ifndef	ACL_TEST_GLOBAL_INCLUDE_H
#define	ACL_TEST_GLOBAL_INCLUDE_H

# ifdef	__plusplus
extern "C" {
# endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_loadcfg.h"
#include "../stdlib/acl_array.h"

#include "acl_test_struct.h"

/*----------------------  Internal interface
 * -----------------------------------*/
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

/*------------------ External test callable interface
 * ----------------------------*/

/* in acl_test_cfg.c */

/**
 * Function: Parse configuration file.
 * @param  pathname Configuration file path
 * @return  0 indicates success, -1 indicates failure
 */
ACL_API int aut_cfg_parse(const char *pathname);

/**
 * Function: Print all valid test command structures.
 * @return Success: 0, Failure: -1
 */
ACL_API int aut_cfg_print(void);

/* in acl_test_misc.c */

ACL_API AUT_LINE *aut_line_new(const ACL_CFG_LINE *cfg_line);

/**
 * Function: Get the corresponding command value and parameter
 * list relationship, displayed as dynamic array.
 * @param cmd_name Command name
 * @return Success: non-NULL pointer, Failure: NULL pointer
 */
ACL_API const ACL_ARRAY *aut_args_get(const char *cmd_name);

/**
 * Function: Get the number of valid test command structures.
 * @return Success: >0, Failure: -1
 */
ACL_API int aut_size(void);

/**
 * Function: Get the corresponding valid test line structure
 * pointer by index value.
 * @param idx Index value
 * @return Success: non-NULL structure pointer, Failure: NULL pointer
 *
 */
ACL_API AUT_LINE *aut_index(int idx);

/**
 * Function: Compare whether the command name is the same as the
 * command name in test_line.
 * @param test_line Structure pointer returned by a valid test command
 * @param cmd_name Command name to query
 * @return If same, returns 0; if different, returns non-0
 */
ACL_API int aut_line_cmdcmp(const AUT_LINE *test_line, const char *cmd_name);

/**
 * Function: Compare whether the execution result value is the
 * same as the result value in the configuration file.
 * @param test_line Structure pointer returned by a valid test command
 * @param result Execution result value of a certain command
 * @return If same, returns 0; if different, returns non-0
 */
ACL_API int aut_line_resultcmp(const AUT_LINE *test_line, int result);

/**
 * Function: Get the line number position of this valid test
 * command in the configuration file.
 * @param test_line Structure pointer returned by a valid test command
 * @return Success: >=0, value is the line number, Failure: < 0
 */
ACL_API int aut_line_number(const AUT_LINE *test_line);

/**
 * Function: Get the valid line number in the configuration file.
 * @param test_line Structure pointer returned by a valid test command
 * @return Success: >=0, value is the line number, Failure: < 0
 */
ACL_API int aut_line_valid_linenum(const AUT_LINE *test_line);

/**
 * Function: Get the command name in the structure.
 * @param test_line Structure pointer returned by a valid test command
 * @return If correct, returns non-0; if error, returns 0
 */
ACL_API const char *aut_line_cmdname(const AUT_LINE *test_line);

/**
 * Function: Get the number of parameters in the structure.
 * @param test_line Structure pointer returned by a valid test command
 * @return Success: >= 0, Failure: -1
 */
ACL_API int aut_line_argc(const AUT_LINE *test_line);

/**
 * Function: Get parameter value in configuration file.
 * @param  test_line AUT_LINE structure pointer
 * @param  name Key name to search for
 * @return Success: non-NULL pointer, Failure: NULL pointer
 */
ACL_API const char *aut_line_getvalue(const AUT_LINE *test_line, const char *name);

/**
 * Function: Get parameters in the structure.
 * @param test_line Structure pointer returned by a valid test command
 * @return Success: non-NULL pointer, Failure: NULL pointer
 */
ACL_API const char *aut_line_argstr(const AUT_LINE *test_line);

/**
 * Function: Get the expected execution result value in the structure.
 * @param test_line Structure pointer returned by a valid test command
 * @return Expected execution result value
 * Note: If no expected result is set, and the parameter is
 * invalid, returns -1, or if unable to parse, returns -1 value.
 *       Whether the value is the expected execution result value
 */
ACL_API int aut_line_result(const AUT_LINE *test_line);

/**
 * Function: Whether there is a stop field in the structure.
 * @param test_line Structure pointer returned by a valid test command
 * @return Yes: 1, No: 0
 */
ACL_API int aut_line_stop(const AUT_LINE *test_line);

/**
 * Function: Internal reserved field, used for internal processing needs.
 * @param test_line Structure pointer returned by a valid test command
 * @return Yes: 1, No: 0
 */
ACL_API int aut_line_reserved(AUT_LINE *test_line);

/**
 * Function: Users can add their own parameters to test_line.
 * @param test_line Structure pointer returned by a valid test command
 * @param arg Parameter to be added by user
 * @return Success: 0, Failure: -1
 */
ACL_API int aut_line_add_arg(AUT_LINE *test_line, void *arg);

/**
 * Function: Delete user's own parameter from test_line.
 * @param test_line Structure pointer returned by a valid test command
 * @param free_fn User's own free function
 */
ACL_API void aut_line_del_arg(AUT_LINE *test_line, void (*free_fn) (void *));

/**
 * Function: Get user's own parameter from test_line.
 * @param test_line Structure pointer returned by a valid test command
 * @return Success: non-NULL pointer, if the pointer is NULL, it
 *  may be that the internal structure is not allocated
 */
ACL_API void *aut_line_get_arg(const AUT_LINE *test_line);

/**
 * Function: Get the end line number in the configuration file.
 * @param start_linenum Line number where loop starts execution
 * @return >= 0 ok; < 0 not found
 */
ACL_API int aut_end_linenum(int start_linenum);

/**
 * Function: Starting from the provided test_line, search forward
 * until finding a structure with the same flag bit as the
 * provided test_line.
 * @param test_line Structure pointer returned by a valid test command
 * @param flag defined as AUT_FLAG_ in acl_test_struct.h
 * @return != NULL, ok find it; == NULL, not found.
 */
ACL_API const AUT_LINE *aut_lookup_from_line(const AUT_LINE *test_line, int flag);

/* in acl_test_runner.c */

/**
 * Execute all registered test functions, if any test execution
 * result does not match the expected result, it will stop
 * execution.
 * @return Success: 0, Failure: -1
 */
ACL_API int aut_start(void);

/**
 * Test process ends, need to call this function to release some
 * memory resources.
 */
ACL_API void aut_stop(void);

/**
 * Function: Register unit test functions to be executed.
 * @param test_fn_tab Unit test function structure array
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

