#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_file.h"
#include "thread/acl_thread.h"
#include "stdlib/acl_token_tree.h"

#endif

#ifndef STR
#define STR	acl_vstring_str
#endif

#ifndef LEN
#define LEN	ACL_VSTRING_LEN
#endif

char *acl_token_delim_tab_new(const char *delim)
{
	char *delim_tab = (char*) acl_mycalloc(255, sizeof(char));
	const unsigned char *ptr = (const unsigned char*) delim;

	while (*ptr) {
		delim_tab[*ptr] = 'd';
		ptr++;
	}

	return (delim_tab);
}

void acl_token_delim_tab_free(char *delim_tab)
{
	acl_myfree(delim_tab);
}

ACL_TOKEN *acl_token_new()
{
	ACL_TOKEN *token = (ACL_TOKEN*) acl_mycalloc(1, sizeof(ACL_TOKEN));
	int   i;

	for (i = 0; i < ACL_TOKEN_WIDTH; i++) {
		token->tokens[i] = NULL;
	}
	token->ch = '-';
	return (token);
}

void acl_token_free(ACL_TOKEN *token)
{
	acl_myfree(token);
}

void acl_token_name(const ACL_TOKEN *token, ACL_VSTRING *buf)
{
	int   i, n;
	char *ptr, *pend, ch;
	const ACL_TOKEN *token_iter;

	ACL_VSTRING_RESET(buf);
	token_iter = token;
	while (token_iter && token_iter->parent != NULL) {
		ACL_VSTRING_ADDCH(buf, token_iter->ch);
		token_iter = token_iter->parent;
	}

	ACL_VSTRING_TERMINATE(buf);

	pend = acl_vstring_end(buf) - 1;
	ptr = STR(buf);
	i = 0;
	n = (pend - ptr + 1) / 2;
	while (i < n) {
		ch = *ptr;
		*ptr = *pend;
		*pend = ch;
		i++;
		ptr++;
		pend--;
	}
}

const char *acl_token_name1(const ACL_TOKEN *token)
{
	static acl_pthread_key_t buf_key = ACL_TLS_OUT_OF_INDEXES;
	ACL_VSTRING *buf;
	static ACL_VSTRING *__buf_unsafe = NULL;

	buf = (ACL_VSTRING*) acl_pthread_tls_get(&buf_key);
	if (buf == NULL) {
		if (buf_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
			if (__buf_unsafe == NULL)
				__buf_unsafe = acl_vstring_alloc(256);
			buf = __buf_unsafe;
		} else {
			buf = acl_vstring_alloc(256);
			acl_pthread_tls_set(buf_key, buf,
				(void (*)(void*)) acl_vstring_free);
		}
	}
	ACL_VSTRING_RESET(buf);
	acl_token_name(token, buf);
	return (STR(buf));
}

ACL_TOKEN *acl_token_tree_add(ACL_TOKEN *token_tree, const char *word,
	unsigned int flag, const void *ctx)
{
	const char *myname= "acl_token_tree_add";
	const unsigned char *ptr = (const unsigned char*) word;
	ACL_TOKEN *token_iter = token_tree, *token = NULL;

	if ((flag & ACL_TOKEN_F_PASS) && (flag & ACL_TOKEN_F_DENY)) {
		acl_msg_error("%s(%d): word(%s)'s flag(%u) is"
			" ACL_TOKEN_F_DENY | ACL_TOKEN_F_PASS",
			myname, __LINE__, word, flag);
		return (NULL);
	}

	while (*ptr) {
		token = token_iter->tokens[*ptr];
		if (token == NULL) {
			token = acl_token_new();
			token->ch = *ptr;
			token_iter->tokens[*ptr] = token;
			token->parent = token_iter;
			ptr++;
			token_iter = token;
		} else if (token->ch != *ptr) {
			acl_msg_fatal("%s(%d): token->ch(%d) != %d",
				myname, __LINE__, token->tokens[*ptr]->ch, *ptr);
		} else {
			ptr++;
			token_iter = token;
		}
	}

	if (token) {
		token->flag = flag;
		token->ctx = ctx;
	}
	return (token);
}

ACL_TOKEN *acl_token_tree_add_word_map(ACL_TOKEN *token_tree,
	const char *word, const char *word_map, unsigned int flag)
{
	const char *myname = "acl_token_tree_add_word_map";
	const unsigned char *ptr = (const unsigned char*) word;
	const unsigned char *ptr_map = (const unsigned char*) word_map;
	ACL_TOKEN *token_iter = token_tree, *token = NULL;

	if ((flag & ACL_TOKEN_F_PASS) && (flag & ACL_TOKEN_F_DENY)) {
		acl_msg_error("%s(%d): word(%s)'s flag(%u) is "
			"ACL_TOKEN_F_DENY | ACL_TOKEN_F_PASS",
			myname, __LINE__, word, flag);
		return (NULL);
	}

	while (*ptr) {
		token = token_iter->tokens[*ptr];
		if (token == NULL) {
			token = acl_token_new();
			token->ch = *ptr_map;
			token_iter->tokens[*ptr] = token;
			token->parent = token_iter;
			ptr++;
			ptr_map++;
			token_iter = token;
		} else if (token->ch != *ptr_map) {
			acl_msg_fatal("%s(%d): token->ch(%d) != %d",
				myname, __LINE__, token->tokens[*ptr]->ch, *ptr_map);
		} else {
			ptr++;
			ptr_map++;
			token_iter = token;
		}
	}

	if (token)
		token->flag = flag;
	return (token);
}

const ACL_TOKEN *acl_token_tree_word_match(const ACL_TOKEN *token_tree, const char *word)
{
	const unsigned char *ptr = (const unsigned char*) word;
	const ACL_TOKEN *token_iter = token_tree, *token = NULL;

	while (*ptr) {
		token = token_iter->tokens[*ptr];
		if (token == NULL)
			return (NULL);
		token_iter = token;
		ptr++;
	}

	if (token && (token->flag & ACL_TOKEN_F_STOP))
		return (token);
	else
		return (NULL);
}

const ACL_TOKEN *acl_token_tree_match(const ACL_TOKEN *token_tree,
	const char **ptr, const char *delim, const char *delim_tab)
{
	const ACL_TOKEN *token_iter = token_tree, *token, *token_last = NULL;
	int   i, n = 0, is_word;

	if (delim) {
		while (**ptr) {
			is_word = *((const unsigned char*) *ptr) > 128 ? 1: 0;
			n++;
			for (i = 0; delim[i]; i++) {
				if (**ptr == delim[i])
					goto END;
			}
			token = token_iter->tokens[*((const unsigned char*) *ptr)];
			if (token == NULL) {
				if (token_last)
					break;
				token_iter = token_tree;
				token = token_iter->tokens[*((const unsigned char*) *ptr)];
				if (token == NULL) {
					(*ptr)++;
					if (**ptr == 0)
						break;
					if (is_word)
						(*ptr)++;
				}
			}
			if (token == NULL)
				continue;
			else if (is_word == 0) {
				if ((token->flag & ACL_TOKEN_F_STOP))
					token_last = token;
				token_iter = token;
				(*ptr)++;
				continue;
			}

			token_iter = token;
			(*ptr)++;
			if (**ptr == 0)
				break;
			token = token_iter->tokens[*((const unsigned char*) *ptr)];
			if (token == NULL) {
				if (token_last)
					break;
				token_iter = token_tree;
			} else if ((token->flag & ACL_TOKEN_F_STOP)) {
				token_last = token;
				token_iter = token;
			} else
				token_iter = token;
			(*ptr)++;
		}
	} else if (delim_tab) {
		while (**ptr) {
			is_word = *((const unsigned char*) *ptr) > 128 ? 1: 0;
			n++;
			if (delim_tab[*((const unsigned char*) *ptr)])
				goto END;
			token = token_iter->tokens[*((const unsigned char*) *ptr)];
			if (token == NULL) {
				if (token_last)
					break;
				token_iter = token_tree;
				token = token_iter->tokens[*((const unsigned char*) *ptr)];
				if (token == NULL) {
					(*ptr)++;
					if (**ptr == 0)
						break;
					if (is_word)
						(*ptr)++;
				}
			}
			if (token == NULL)
				continue;
			else if (is_word == 0) {
				if ((token->flag & ACL_TOKEN_F_STOP))
					token_last = token;
				token_iter = token;
				(*ptr)++;
				continue;
			}

			token_iter = token;
			(*ptr)++;
			if (**ptr == 0)
				break;
			token = token_iter->tokens[*((const unsigned char*) *ptr)];
			if (token == NULL) {
				if (token_last)
					break;
				token_iter = token_tree;
			} else if ((token->flag & ACL_TOKEN_F_STOP)) {
				token_last = token;
				token_iter = token;
			} else
				token_iter = token;
			(*ptr)++;
		}
	} else {
		while (**ptr) {
			is_word = *((const unsigned char*) *ptr) > 128 ? 1: 0;
			n++;
			token = token_iter->tokens[*((const unsigned char*) *ptr)];
			if (token == NULL) {
				if (token_last)
					break;
				token_iter = token_tree;
				token = token_iter->tokens[*((const unsigned char*) *ptr)];
				if (token == NULL) {
					(*ptr)++;
					if (**ptr == 0)
						break;
					if (is_word)
						(*ptr)++;
				}
			}
			if (token == NULL)
				continue;
			else if (is_word == 0) {
				if ((token->flag & ACL_TOKEN_F_STOP))
					token_last = token;
				token_iter = token;
				(*ptr)++;
				continue;
			}

			token_iter = token;
			(*ptr)++;
			if (**ptr == 0)
				break;
			token = token_iter->tokens[*((const unsigned char*) *ptr)];
			if (token == NULL) {
				if (token_last)
					break;
				token_iter = token_tree;
			} else if ((token->flag & ACL_TOKEN_F_STOP)) {
				token_last = token;
				token_iter = token;
			} else
				token_iter = token;
			(*ptr)++;
		}
	}

END:
	return (token_last);
}

void acl_token_tree_walk(const ACL_TOKEN *token_tree,
	void (*walk_fn)(const ACL_TOKEN*, void*), void *arg)
{
	int   i;

	if ((token_tree->flag & ACL_TOKEN_F_STOP))
		walk_fn(token_tree, arg);

	for (i = 0; i < ACL_TOKEN_WIDTH; i++) {
		if (token_tree->tokens[i]) {
			acl_token_tree_walk(token_tree->tokens[i], walk_fn, arg);
		}
	}
}

static void acl_token_name_walk(const ACL_TOKEN *token, void *arg)
{
	ACL_VSTRING *buf = (ACL_VSTRING*) arg;
	ACL_VSTRING *name = acl_vstring_alloc(256);

	acl_token_name(token, name);
	if (LEN(buf) > 0)
		ACL_VSTRING_ADDCH(buf, ';');
	acl_vstring_strcat(buf, STR(name));
	acl_vstring_free(name);
}

void acl_token_tree_print(const ACL_TOKEN *token_tree)
{
	int   i;
	ACL_VSTRING *buf = acl_vstring_alloc(1024);

	for (i = 0; i < ACL_TOKEN_WIDTH; i++) {
		if (token_tree->tokens[i]) {
			acl_token_tree_walk(token_tree->tokens[i], acl_token_name_walk, buf);
		}
	}

	printf(">>>all token: (%s)\n", STR(buf));
	acl_vstring_free(buf);
}

ACL_TOKEN *acl_token_tree_create(const char *s)
{
	return (acl_token_tree_create2(s, ";, \t"));
}

ACL_TOKEN *acl_token_tree_create2(const char *s, const char *sep)
{
	const char *myname = "acl_token_tree_create";
	ACL_ARGV *argv;
	ACL_ITER  iter;
	unsigned int   flag;
	ACL_TOKEN *token_tree;
	const ACL_TOKEN *token;
	ACL_VSTRING *buf;

	token_tree = acl_token_new();
	if (s == NULL || *s == 0)
		return (token_tree);

	buf = acl_vstring_alloc(256);
	argv = acl_argv_split(s, sep);
	acl_foreach(iter, argv) {
		char *word = (char*) iter.ptr;
		char *ptr = strchr(word, '|');

		flag = ACL_TOKEN_F_STOP;
		if (ptr) {
			*ptr++ = 0;
			if (*ptr == 'D' || *ptr == 'd')
				flag |= ACL_TOKEN_F_DENY;
			if (*ptr == 'P' || *ptr == 'p')
				flag |= ACL_TOKEN_F_PASS;
		}
		token = acl_token_tree_add(token_tree, word, flag, NULL);
		if (token == NULL) {
			acl_msg_info("%s(%d): word(%s) discard",
				myname, __LINE__, word);
		} else {
			ACL_VSTRING_RESET(buf);
			acl_token_name(token, buf);
			/*
			acl_msg_info("%s(%d): add word(%s) ok, token's name(%s)",
				myname, __LINE__, word, STR(buf));
			*/
		}
	}

	acl_argv_free(argv);
	acl_vstring_free(buf);
	return (token_tree);
}

void acl_token_tree_destroy(ACL_TOKEN *token_tree)
{
	int   i;
	/*
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	*/

	for (i = 0; i < ACL_TOKEN_WIDTH; i++) {
		if (token_tree->tokens[i])
			acl_token_tree_destroy(token_tree->tokens[i]);
	}

	/*
	acl_token_name(token_tree, buf);
	if ((token_tree->flag & ACL_TOKEN_F_STOP))
		acl_msg_info("free token: %s", STR(buf));
	else
		printf("free token; ");
	acl_vstring_free(buf);
	*/
	acl_token_free(token_tree);
}

void acl_token_tree_load_deny(const char *filepath, ACL_TOKEN *token_tree)
{
	const char *myname = "acl_token_tree_load_deny";
	ACL_FILE *fp;
	int   flag;
	char  buf[1024], *ptr;

	if (!filepath || !*filepath)
		return;
	fp = acl_fopen(filepath, "r");
	if (fp == NULL) {
		acl_msg_warn("%s(%d): %s open error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return;
	}

	while (1) {
		ptr = acl_fgets_nonl(buf, sizeof(buf), fp);
		if (ptr == NULL)
			break;
		ptr = strchr(buf, '|');
		if (ptr) {
			*ptr++ = 0;
			flag = ACL_TOKEN_F_STOP;
			if (*ptr == 'd' || *ptr == 'D')
				flag |= ACL_TOKEN_F_DENY;
			if (*ptr == 'p' || *ptr == 'P')
				flag |= ACL_TOKEN_F_PASS;
		} else
			flag = ACL_TOKEN_F_STOP | ACL_TOKEN_F_DENY;
		acl_token_tree_add(token_tree, buf, flag, NULL);
	}

	acl_fclose(fp);
}

void acl_token_tree_load_pass(const char *filepath, ACL_TOKEN *token_tree)
{
	const char *myname = "acl_token_tree_load_pass";
	ACL_FILE *fp;
	int   flag;
	char  buf[1024], *ptr;

	if (!filepath || !*filepath)
		return;
	fp = acl_fopen(filepath, "r");
	if (fp == NULL) {
		acl_msg_warn("%s(%d): %s open error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return;
	}

	while (1) {
		ptr = acl_fgets_nonl(buf, sizeof(buf), fp);
		if (ptr == NULL)
			break;
		ptr = strchr(buf, ':');
		if (ptr) {
			*ptr++ = 0;
			flag = ACL_TOKEN_F_STOP;
			if (*ptr == 'd' || *ptr == 'D')
				flag |= ACL_TOKEN_F_DENY;
			if (*ptr == 'p' || *ptr == 'P')
				flag |= ACL_TOKEN_F_PASS;
		} else
			flag = ACL_TOKEN_F_STOP | ACL_TOKEN_F_PASS;
		acl_token_tree_add(token_tree, buf, flag, NULL);
	}

	acl_fclose(fp);
}

static void token_word_test(const char *tokens, const char *test_tab[])
{
	ACL_TOKEN *token_tree;
	const char *ptr;
	int   i;

	token_tree = acl_token_tree_create(tokens);
	acl_token_tree_print(token_tree);

	for (i = 0; test_tab[i] != NULL; i++) {
		ptr = test_tab[i];
		printf("match %s %s\n", ptr,
			acl_token_tree_word_match(token_tree, ptr) ? "yes" : "no");
	}
	acl_token_tree_destroy(token_tree);
}

static void token_tree_test(const char *tokens, const char *test_tab[])
{
	ACL_TOKEN *token_tree;
	const ACL_TOKEN *token;
	const char *ptr, *psaved;
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	int   i;

	token_tree = acl_token_tree_create(tokens);
	acl_token_tree_print(token_tree);

	for (i = 0; test_tab[i] != NULL; i++) {
		ptr = psaved = test_tab[i];
		token = acl_token_tree_match(token_tree, &ptr, ";", NULL);
		if (token) {
			ACL_VSTRING_RESET(buf);
			acl_token_name(token, buf);
			printf("match %s %s, token's name: %s\n", psaved,
					(token->flag & ACL_TOKEN_F_DENY) ? "DENY"
					: (token->flag & ACL_TOKEN_F_PASS ? "PASS" : "NONE"),
					STR(buf));
		} else
			printf("match %s none\n", psaved);
	}

	acl_token_tree_destroy(token_tree);
	acl_vstring_free(buf);
}

void acl_token_tree_test(void)
{
	const char *tokens1 = "hello world he is a man he"
		" 中 中华 中华人 中华人民 中华人民共 中华人民共和 中华人民共和国"
		" 中华人民共和国万岁 中华人民共和国万岁万万岁"
		" 法轮功|d 研究法轮功|d 反对法轮功|p 法轮功协会|d 口交|d 二十四口交换机|p";
	const char *tokens2 = "法轮功|d 研究法轮功|d 比利时|d 中国|p 说的|d";

	static const char *test1_tab[] = {
		"中华",
		"中华人",
		"中华人民",
		"中华人民共",
		"中华人民共和",
		"中华人民共和国",
		"中华人民共和国万岁",
		"我们中华人民共和国万岁",
		"我们中华人民共和国万岁万万岁",
		"法轮功",
		"反对法轮功",
		"法轮功协会",
		"反对法轮功协会",
		"研究法轮功",
		"我爱法轮功",
		"我爱研法轮功",
		"口交",
		"男女口交",
		"二十四口交换机",
		NULL
	};

	static const char *test2_tab[] = {
		"hello",
		"shello",
		"中华人民共和国",
		"中华人民",
		NULL
	};

	static const char *test3_tab[] = {
		"我爱研法轮功",
		"中国",
		"比利时",
		"中国比利时",
		"我说的故事",
		"宜档闹泄",
		NULL
	};

	token_tree_test(tokens1, test1_tab);
	token_word_test(tokens1, test2_tab);
	token_tree_test(tokens2, test3_tab);
}
