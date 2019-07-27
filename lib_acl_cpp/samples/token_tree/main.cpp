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
	"涓浗",
	"涓浗浜烘皯",
	"涓浗浜烘皯閾惰",
	"涓浗浜烘皯瑙ｆ斁鍐�",
	NULL,
};

static const char *tokens2[] = {
	"涓崕",
	"涓崕浜�",
	"涓崕浜烘皯",
	"涓崕浜烘皯鍏�",
	"涓崕浜烘皯鍏卞拰",
	"涓崕浜烘皯鍏卞拰鍥�",
	"涓崕浜烘皯鍏卞拰鍥戒竾宀�",
	"鎴戜滑涓崕浜烘皯鍏卞拰鍥戒竾宀�",
	"鎴戜滑涓崕浜烘皯鍏卞拰鍥戒竾宀佷竾涓囧瞾",
	NULL
};

static const char *tokens3[] = {
	"hello",
	"shello",
	"涓崕浜烘皯鍏卞拰鍥�",
	"涓崕浜烘皯",
	NULL
};

static const char *tokens4[] = {
	"涓浗",
	"姣斿埄鏃�",
	"涓浗姣斿埄鏃�",
	"鎴戣鐨勬晠浜�",
	"瀹滄。闂规硠",
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
