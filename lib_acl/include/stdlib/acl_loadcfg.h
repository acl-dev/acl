#ifndef	ACL_LOADCFG_INCLUDE_H
#define	ACL_LOADCFG_INCLUDE_H

# ifdef	__cplusplus
extern "C" {
# endif

#include "acl_define.h"
#include "acl_array.h"

typedef struct ACL_CFG_PARSER ACL_CFG_PARSER;

typedef struct ACL_CFG_LINE {
	char **value;	/* store the separated values of the line */
	char *pdata;	/* point to the beginning of the line data */
	int   ncount;	/* how many values in this line data,
			 * if ncount == 0, then this is an
			 * invalid line, but still be stored
			 * here in pdata.
			 */
	int   line_number;	/* 该行在配置文件中的行号 */
} ACL_CFG_LINE;

typedef struct ACL_CFG_FN {
	const char  *name;
	int   (*func)(const ACL_CFG_LINE *);
} ACL_CFG_FN;

typedef void (*ACL_CFG_WALK_FN)(void *arg);

ACL_API ACL_CFG_PARSER *acl_cfg_parser_load(const char *pathname, const char *delimiter);
ACL_API void acl_cfg_parser_free(ACL_CFG_PARSER *parser);
ACL_API void acl_cfg_parser_walk(ACL_CFG_PARSER *parser, ACL_CFG_WALK_FN walk_fn);
ACL_API int acl_cfg_line_replace(ACL_CFG_LINE *cfg_line, const char **value, int from, int to);
ACL_API ACL_CFG_LINE *acl_cfg_parser_index(const ACL_CFG_PARSER *parser, int idx);
ACL_API int acl_cfg_parser_size(const ACL_CFG_PARSER *parser);
ACL_API int acl_cfg_parser_dump(const ACL_CFG_PARSER *parser, const char *pathname, const char *delimiter);
ACL_API int acl_cfg_parser_append(ACL_CFG_PARSER *parser, ACL_CFG_LINE *cfg_line);
ACL_API int acl_cfg_parser_delete(ACL_CFG_PARSER *parser, const char *name);
ACL_API ACL_CFG_LINE *acl_cfg_line_new(const char **value, int ncount);

# ifdef	__cplusplus
}
# endif

#endif

