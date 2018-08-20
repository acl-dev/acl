#include "stdafx.h"
#include "util.h"

static void test1(const char* tokens[])
{
	acl::token_tree tree;

	for (size_t i = 0; tokens[i] != NULL; i++) {
		printf("add token=%s\r\n", tokens[i]);
		tree.insert(tokens[i]);
	}

	printf("======================================================\r\n");
	for (size_t i = 0; tokens[i] != NULL; i++) {
		const acl::token_node* node = tree.find(tokens[i]);
		if (node == NULL) {
			printf("can't found %s\r\n", tokens[i]);
			break;
		}
		printf("ok, find it, key=%s\r\n", node->get_key());
	}
}

static void test2(void)
{
	const char* disks = "/data1; /data2; /data3; /data4; /data; /data5;"
		" /data6; /data7; /data8; /data9; /data10; /data10/www";
	acl::string buf(disks);
	std::vector<acl::string>& tokens = buf.split2(";, \t\r\n");
	acl::token_tree tree;
	for (std::vector<acl::string>::const_iterator cit = tokens.begin();
		cit != tokens.end(); ++cit) {
		tree.insert(*cit);
	}

	const char* path = "/data10/www/xxxx", *ptr = path;
	const acl::token_node* node = tree.search(&ptr);
	if (node) {
		printf("ok, found it, key=%s, path=%s\r\n",
			node->get_key(), path);
	} else {
		printf("not found, path=%s\r\n", path);
	}

	node = tree.find(path);
	if (node) {
		printf("error, we found it, path=%s, key=%s\r\n",
			path, node->get_key());
	} else {
		printf("ok, not found, key=%s\r\n", path);
	}

	path = "/data10";
	node = tree.find(path);

	if (node) {
		printf("ok, found it, path=%s, key=%s\r\n",
			path, node->get_key());
	} else {
		printf("error, not found, path=%s\r\n", path);
	}
}

static const char *tokens1[] = {
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

static const char *tokens3[] = {
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

int main(void)
{
	acl::acl_cpp_init();
	test1(tokens1);
	printf("-----------------------------------------------------\r\n");
	test1(tokens2);
	printf("-----------------------------------------------------\r\n");
	test1(tokens3);
	printf("-----------------------------------------------------\r\n");
	test1(tokens4);
	printf("-----------------------------------------------------\r\n");
	test2();
	printf("-----------------------------------------------------\r\n");
	return 0;
}
