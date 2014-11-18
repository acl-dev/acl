#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#define	STR	acl_vstring_str

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

static const char *test_tab[] = {
	"中国"
	"中国人民",
	"中国人民银行",
	"中国人民解放军",
	NULL
};

static void test(void)
{
	const char *tokens = "中国|p 中国人|p 中国人民|p 中国人民银行|p";

	token_tree_test(tokens, test_tab);
}

static void test2(void)
{
	ACL_TOKEN *tree;
	const ACL_TOKEN *token;
	const char *n1 = "名称1", *n2 = "名称2", *n3 = "名称3";
	const char *v1 = "变量1", *v2 = "变量2", *v3 = "变量3";
	const char *s = "中国人民名称1，在这个世界上，你在哪儿名称2? "
		"我不知道你的名称，你能告诉我吗？我的名称3是...";
	const char *p = s;

	if (1)
	{
		tree = acl_token_tree_create(NULL);
		acl_token_tree_add(tree, n1, ACL_TOKEN_F_STOP, v1);
		acl_token_tree_add(tree, n2, ACL_TOKEN_F_STOP, v2);
		acl_token_tree_add(tree, n3, ACL_TOKEN_F_STOP, v3);
	}
	else
		tree = acl_token_tree_create("名称1|p 名称2|p 名称3|p");

	printf("-----------------------------------\n");

	acl_token_tree_print(tree);
	token = acl_token_tree_word_match(tree, "名称1");
	if (token)
		printf("find, %s: %s\n", acl_token_name1(token),
			(const char*) token->ctx);
	else
		printf("no find\n");

	printf("-----------------------------------\n");

	while (*p) {
		token = acl_token_tree_match(tree, &p, NULL, NULL);
		if (token == NULL)
			break;
		printf("%s: %s\n", acl_token_name1(token),
			(const char*) token->ctx);
	}

	printf("-----------------------------------\n");

	acl_token_tree_destroy(tree);
}

int main(void)
{
	test();
#if	0
	acl_token_tree_test();
#endif

	test2();
#ifdef ACL_MS_WINDOWS
	printf("enter any key to exit ...\n");

	getchar();
#endif
	return (0);
}
