#ifndef ACL_TOKEN_TREE_INCLUDE_H
#define ACL_TOKEN_TREE_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "acl_define.h"
#include "acl_vstring.h"

#define ACL_PRINT_CHAR(x)  \
	((((x) >= 'a' && (x) <='z')  \
	|| ((x) >= 'A' && (x) <= 'Z')  \
	|| ((x) >= '0' && (x) <= '9')  \
	|| (x) == ';' || (x) == '!'  \
	|| (x) == ':' || (x) == ','  \
	|| (x) == '.' || (x) == '@'  \
	|| (x) == '#' || (x) == '$'  \
	|| (x) == '%' || (x) == '^'  \
	|| (x) == '&' || (x) == '*'  \
	|| (x) == '(' || (x) == ')'  \
	|| (x) == '-' || (x) == '='  \
	|| (x) == '|' || (x) == '\\'  \
	|| (x) == '[' || (x) == ']'  \
	|| (x) == '{' || (x) == '}'  \
	|| (x) == '\'' || (x) == '"')  \
	? (x) : '-')

typedef struct ACL_TOKEN {
	unsigned char  ch;
	unsigned int  flag;
#define ACL_TOKEN_F_NONE	0
#define ACL_TOKEN_F_STOP	(1 << 0)
#define ACL_TOKEN_F_PASS	(1 << 1)
#define ACL_TOKEN_F_DENY	(1 << 2)
#define ACL_TOKEN_F_UTF8	(1 << 3)

#define ACL_TOKEN_WIDTH		255
	struct ACL_TOKEN *tokens[ACL_TOKEN_WIDTH];
	struct ACL_TOKEN *parent;
	const void *ctx;
} ACL_TOKEN;

#define ACL_TOKEN_TREE_MATCH(acl_token_tree_in, s_in, delim_in, delim_tab_in, acl_token_out) do  \
{  \
	const ACL_TOKEN *acl_token_iter = (acl_token_tree_in), *acl_token_tmp; \
	(acl_token_out) = NULL;  \
	if (((const char*) delim_in)) {  \
		int   _i;  \
		while (*(s_in)) {  \
			for (_i = 0; ((const char*) delim_in)[_i]; _i++) {  \
				if (*(s_in) == ((const char*) delim_in)[_i])  \
					goto _END;  \
			}  \
			acl_token_tmp = acl_token_iter->tokens[*((const unsigned char*)(s_in))];  \
			if (acl_token_tmp == NULL) {  \
				if ((acl_token_out))  \
					break;  \
				acl_token_iter = (acl_token_tree_in);  \
				acl_token_tmp = acl_token_iter->tokens[*((const unsigned char*)(s_in))];  \
				if (acl_token_tmp == NULL) {  \
					(s_in)++;  \
					continue;  \
				}  \
			}  \
			if ((acl_token_tmp->flag & ACL_TOKEN_F_STOP))  \
				(acl_token_out) = acl_token_tmp;  \
			acl_token_iter = acl_token_tmp;  \
			(s_in)++;  \
		}  \
_END:  \
		break;  \
	} else if (((char*) delim_tab_in)) {  \
		while (*(s_in)) {  \
			if (((char*) delim_tab_in)[*((const unsigned char*)(s_in))])  \
				break;  \
			acl_token_tmp = acl_token_iter->tokens[*((const unsigned char*)(s_in))];  \
			if (acl_token_tmp == NULL) {  \
				if ((acl_token_out))  \
					break;  \
				acl_token_iter = (acl_token_tree_in);  \
				acl_token_tmp = acl_token_iter->tokens[*((const unsigned char*)(s_in))];  \
				if (acl_token_tmp == NULL) {  \
					(s_in)++;  \
					continue;  \
				}  \
			}  \
			if ((acl_token_tmp->flag & ACL_TOKEN_F_STOP))  \
				(acl_token_out) = acl_token_tmp;  \
			acl_token_iter = acl_token_tmp;  \
			(s_in)++;  \
		}  \
	} else {  \
		while (*(s_in)) {  \
			acl_token_tmp = acl_token_iter->tokens[*((const unsigned char*)(s_in))];  \
			if (acl_token_tmp == NULL) {  \
				if ((acl_token_out))  \
					break;  \
				acl_token_iter = (acl_token_tree_in);  \
				acl_token_tmp = acl_token_iter->tokens[*((const unsigned char*)(s_in))];  \
				if (acl_token_tmp == NULL) {  \
					(s_in)++;  \
					continue;  \
				}  \
			}  \
			if ((acl_token_tmp->flag & ACL_TOKEN_F_STOP))  \
				(acl_token_out) = acl_token_tmp;  \
			acl_token_iter = acl_token_tmp;  \
			(s_in)++;  \
		}  \
	}  \
} while (0)

ACL_API char *acl_token_delim_tab_new(const char *delim);
ACL_API void acl_token_delim_tab_free(char *delim_tab);
ACL_API ACL_TOKEN *acl_token_new(void);
ACL_API void acl_token_free(ACL_TOKEN *token);
ACL_API void acl_token_name(const ACL_TOKEN *token, ACL_VSTRING *buf);
ACL_API const char *acl_token_name1(const ACL_TOKEN *token);
ACL_API ACL_TOKEN *acl_token_tree_add(ACL_TOKEN *token_tree,
	const char *word, unsigned int flag, const void *ctx);
ACL_API ACL_TOKEN *acl_token_tree_add_word_map(ACL_TOKEN *token_tree,
	const char *word, const char *word_map, unsigned int flag);
ACL_API const ACL_TOKEN *acl_token_tree_word_match(const ACL_TOKEN *token_tree,
	const char *word);
ACL_API const ACL_TOKEN *acl_token_tree_match(const ACL_TOKEN *token_tree,
	const char **ptr, const char *delim, const char *delim_tab);
ACL_API void acl_token_tree_walk(const ACL_TOKEN *token_tree,
	void (*walk_fn)(const ACL_TOKEN*, void*), void *arg);
ACL_API void acl_token_tree_print(const ACL_TOKEN *token_tree);
ACL_API ACL_TOKEN *acl_token_tree_create(const char *s);
ACL_API ACL_TOKEN *acl_token_tree_create2(const char *s, const char *sep);
ACL_API void acl_token_tree_destroy(ACL_TOKEN *token_tree);
ACL_API void acl_token_tree_load_deny(const char *filepath, ACL_TOKEN *token_tree);
ACL_API void acl_token_tree_load_pass(const char *filepath, ACL_TOKEN *token_tree);
ACL_API void acl_token_tree_test(void);

#ifdef __cplusplus
}
#endif

#endif
