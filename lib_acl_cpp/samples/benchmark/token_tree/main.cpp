#include "stdafx.h"

static const char *tokens[] = {
	"中华",
	"中华人",
	"中华人民",
	"中华人民共",
	"中华人民共和",
	"中华人民共和国",
	"中华人民共和国万岁",
	"我们中华人民共和国万岁",
	"我们中华人民共和国万岁万万岁",
	NULL
};

static void test_token_tree_c(int max)
{
	ACL_TOKEN* tree = acl_token_tree_create(NULL);

	for (int i = 0; tokens[i] != NULL; i++) {
		acl_token_tree_add(tree, tokens[i], ACL_TOKEN_F_STOP, NULL);
	}

	acl::meter_time(__FUNCTION__, __LINE__, "begin\t");

	int j = 0;
	for (int i = 0; i < max; i++) {
		ACL_TOKEN* token = acl_token_tree_word_match(tree, tokens[j]);
		if (token == NULL) {
			printf("find error, key=%s\r\n", tokens[j]);
			break;
		}
		if (tokens[++j] == NULL) {
			j = 0;
		}
	}

	acl::meter_time(__FUNCTION__, __LINE__, "end\t\t");

	acl_token_tree_destroy(tree);
}

static void test_token_tree_cpp(int max)
{
	acl::token_tree tree;

	for (int i = 0; tokens[i] != NULL; i++) {
		tree.insert(tokens[i]);
	}

	acl::meter_time(__FUNCTION__, __LINE__, "begin\t");

	int j = 0;
	for (int i = 0; i < max; i++) {
		const acl::token_node* node = tree.find(tokens[j]);
		if (node == NULL) {
			printf("find error, key=%s\r\n", tokens[j]);
			break;
		}
		if (tokens[++j] == NULL) {
			j = 0;
		}
	}

	acl::meter_time(__FUNCTION__, __LINE__, "end\t");
}

static void test_htable(int max)
{
	ACL_HTABLE* htable = acl_htable_create(100, 0);

	for (int i = 0; tokens[i] != NULL; i++) {
		acl_htable_enter(htable, tokens[i], (void*) tokens[i]);
	}

	acl::meter_time(__FUNCTION__, __LINE__, "begin\t");

	int j = 0;
	for (int i = 0; i < max; i++) {
		const char* value = (const char*) acl_htable_find(htable, tokens[j]);
		if (value == NULL) {
			printf("find error, key=%s\r\n", tokens[j]);
			break;
		}
		if (tokens[++j] == NULL) {
			j = 0;
		}
	}

	acl::meter_time(__FUNCTION__, __LINE__, "end\t");
	acl_htable_free(htable, NULL);
}

static void test_stdmap(int max)
{
	std::map<std::string, bool> map;

	for (int i = 0; tokens[i] != NULL; i++) {
		map[tokens[i]] = true;
	}

	acl::meter_time(__FUNCTION__, __LINE__, "begin\t");

	int j = 0;
	for (int i = 0; i < max; i++) {
		if (map.find(tokens[j]) == map.end()) {
			printf("find error, key=%s\r\n", tokens[j]);
			break;
		}
		if (tokens[++j] == NULL) {
			j = 0;
		}
	}

	acl::meter_time(__FUNCTION__, __LINE__, "end\t");
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -n max[100]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, max = 100;

	while ((ch = getopt(argc, argv, "hn:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	test_token_tree_cpp(max);
	printf("-------------------------------------------------------\r\n");
	test_token_tree_c(max);
	printf("-------------------------------------------------------\r\n");
	test_htable(max);
	printf("-------------------------------------------------------\r\n");
	test_stdmap(max);
	printf("-------------------------------------------------------\r\n");

	return 0;
}
