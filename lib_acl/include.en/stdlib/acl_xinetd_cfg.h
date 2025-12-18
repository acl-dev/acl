#ifndef	ACL_XINETD_CFG_INCLUDE_H
#define	ACL_XINETD_CFG_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_array.h"

/**
 * Configuration file parser object type definition.
 */
typedef struct ACL_XINETD_CFG_PARSER ACL_XINETD_CFG_PARSER;

/**
 * Function: Get configuration item's value from configuration file parser.
 * @param xcp Structure pointer, must not be NULL
 * @param name Configuration item's name
 * @return Configuration file parser's configuration item value
 */
ACL_API const char *acl_xinetd_cfg_get(const ACL_XINETD_CFG_PARSER *xcp, const char *name);

/**
 * Function: Get configuration item's value array from configuration file parser.
 * When a configuration item corresponds to multiple values, use this.
 * @param xcp Structure pointer, must not be NULL
 * @param name Configuration item's name
 * @return Configuration file parser's configuration item data dynamic array
 */
ACL_API const ACL_ARRAY *acl_xinetd_cfg_get_ex(const ACL_XINETD_CFG_PARSER *xcp, const char *name);

/**
 * Function: Get configuration item's value at a certain index position from
 * configuration file parser.
 * @param xcp Structure pointer, must not be NULL
 * @param idx Index position value
 * @param ppname Pointer to pointer address's variable
 * @param ppvalue Pointer to pointer address's variable
 * @return  0: OK, -1: ERR
 */
ACL_API int acl_xinetd_cfg_index(const ACL_XINETD_CFG_PARSER *xcp,
			int idx,
			char **ppname,
			char **ppvalue);

/**
 * Function: Get configuration file parser's configuration item count.
 * @param xcp Structure pointer, must not be NULL
 * @return Configuration file parser's configuration item count
 */
ACL_API int acl_xinetd_cfg_size(const ACL_XINETD_CFG_PARSER *xcp);

/**
 * Function: Free memory space pointed to by structure pointer.
 * @param xcp Structure pointer
 */
ACL_API void acl_xinetd_cfg_free(ACL_XINETD_CFG_PARSER *xcp);

/**
 * Function: Load and parse configuration file.
 * @param pathname Configuration file's file name
 * @return Already parsed configuration file's structure pointer
 */
ACL_API ACL_XINETD_CFG_PARSER *acl_xinetd_cfg_load(const char *pathname);

/**
 * Integer type configuration table structure.
 */
typedef struct ACL_CFG_INT_TABLE {
	const char *name;
	int  defval;
	int *target;
	int  min;
	int  max;
} ACL_CFG_INT_TABLE;

/**
 * 64-bit integer type configuration table structure.
 */
typedef struct ACL_CFG_INT64_TABLE {
	const char *name;
	acl_int64  defval;
	acl_int64 *target;
	acl_int64  min;
	acl_int64  max;
} ACL_CFG_INT64_TABLE;

/**
 * String type configuration table structure.
 */
typedef struct ACL_CFG_STR_TABLE {
	const char *name;
	const char *defval;
	char **target;
} ACL_CFG_STR_TABLE;

/**
 * Boolean type configuration table structure.
 */
typedef struct ACL_CFG_BOOL_TABLE {
	const char *name;
	int   defval;
	int  *target;
} ACL_CFG_BOOL_TABLE;

/* in acl_xinetd_params.c */

/**
 * Read integer type table from configuration file parser.
 * @param cfg {ACL_XINETD_CFG_PARSER*} When NULL, will assign default values
 * @param table {ACL_CFG_INT_TABLE*}
 */
ACL_API void acl_xinetd_params_int_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_INT_TABLE *table);

/**
 * Read 64-bit integer type table from configuration file parser.
 * @param cfg {ACL_XINETD_CFG_PARSER*} When NULL, will assign default values
 * @param table {ACL_CFG_INT64_TABLE*}
 */
ACL_API void acl_xinetd_params_int64_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_INT64_TABLE *table);

/**
* Read string type table from configuration file parser.
* @param cfg {ACL_XINETD_CFG_PARSER*} When NULL, will assign default values
* @param table {ACL_CFG_STR_TABLE*}
*/
ACL_API void  acl_xinetd_params_str_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_STR_TABLE *table);

/**
* Read BOOL type table from configuration file parser.
* @param cfg {ACL_XINETD_CFG_PARSER*} When NULL, will assign default values
* @param table {ACL_CFG_BOOL_TABLE*}
*/
ACL_API void  acl_xinetd_params_bool_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_BOOL_TABLE *table);

#ifdef	__cplusplus
}
#endif

#endif
