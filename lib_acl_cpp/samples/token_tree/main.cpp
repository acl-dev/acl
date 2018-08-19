#include "stdafx.h"
#include "util.h"

static void test1(const char* tokens[], const char* haystacks[])
{
	acl::token_tree tree;

	for (size_t i = 0; tokens[i] != NULL; i++) {
		printf("add token=%s\r\n", tokens[i]);
		tree.insert(tokens[i]);
	}

	printf("======================================================\r\n");
	for (size_t i = 0; haystacks[i] != NULL; i++) {
		const acl::token_node* node = tree.search(haystacks[i]);
		if (node == NULL) {
			printf("can't found %s\r\n", haystacks[i]);
			break;
		}
		printf("ok, find it, key=%s\r\n", node->get_key());
	}
}

static const char *tokens1[] = {
	"中国",
	"中国人民",
	"中国人民银行",
	"中国人民解放军",
	NULL,
};

static const char *haystack1[] = {
	"中国",
	"中国人民",
	"中国人民银行",
	"中国人民解放军",
	NULL,
};

static const char *tokens2[] = {
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

static const char *haystack2[] = {
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

static const char *tokens3[] = {
	"hello",
	"shello",
	"中华人民共和国",
	"中华人民",
	NULL
};

static const char *haystack3[] = {
	"hello",
	"shello",
	"中华人民共和国",
	"中华人民",
	NULL
};

static const char *tokens4[] = {
	"中国",
	"比利时",
	"中国比利时",
	"我说的故事",
	"宜档闹泄",
	NULL
};

static const char *haystack4[] = {
	"中国",
	"比利时",
	"中国比利时",
	"我说的故事",
	"宜档闹泄",
	NULL
};

int main(void)
{
	acl::acl_cpp_init();
	test1(tokens1, haystack1);
	printf("-----------------------------------------------------\r\n");
	test1(tokens2, haystack2);
	printf("-----------------------------------------------------\r\n");
	test1(tokens3, haystack3);
	printf("-----------------------------------------------------\r\n");
	test1(tokens4, haystack4);
	printf("-----------------------------------------------------\r\n");
	return 0;
}
