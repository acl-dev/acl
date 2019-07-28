#include "lib_acl.h"

#define STR	acl_vstring_str

static const char* default_data = \
    "{ 'menu name': {\r\n"
    "    'id:file': 'file',\r\n"
    "    'value{': 'File',\r\n"
    "    'popup{}': {\r\n"
    "        'menuitem1}': [\r\n"
    "            {'value': 'New', 'onclick': 'CreateNewDoc()'},\r\n"
    "            {'value': 'Open', 'onclick': 'OpenDoc()'},\r\n"
    "            {'value': 'Close', 'onclick': 'CloseDoc()'}\r\n"
    "        ],\r\n"
    "        'menuname[]': 'hello world',\r\n"
    "        'inner': { 'value' : 'new ', 'value' : 'open' },\r\n"
    "        'menuitem2': [\r\n"
    "            {'value': 'New', 'onclick': 'CreateNewDoc()'},\r\n"
    "            {'value': 'Open', 'onclick': 'OpenDoc()'},\r\n"
    "            {'value': 'Close', 'onclick': 'CloseDoc()'},\r\n"
    "            {{'value': 'Help', 'onclick': 'Help()'}}"
    "        ]\r\n"
    "    }\r\n"
    " }\r\n,"
    " 'help': 'hello world!',\r\n"
    " 'menuitem2': [\r\n"
    "   {'value': 'New', 'onclick': 'CreateNewDoc()'},\r\n"
    "   {'value': 'Open', 'onclick': 'OpenDoc()'},\r\n"
    "   {'value': 'Close', 'onclick': 'CloseDoc()'},\r\n"
    "   [{'value': 'Save', 'onclick': 'SaveDoc()'}]"
    " ]\r\n"
    "}\r\n";

static void print_json_node(const ACL_JSON* json, const ACL_JSON_NODE* node)
{
	for (int i = 1; i < node->depth; i++)
		printf("\t");
	printf("tag> %s, parent %s, text: %s, child: %s, type: %u\n",
		STR(node->ltag), node->parent == json->root
		? "root" : STR(node->parent->ltag),
		STR(node->text), node->tag_node ? "yes" : "no",
		node->type);
}

static void test_json_foreach1(ACL_JSON* json)
{
	ACL_ITER iter;

	printf("------------ in %s ------------\r\n", __FUNCTION__);

	acl_foreach(iter, json)
	{
		const ACL_JSON_NODE* node = (const ACL_JSON_NODE*) iter.data;
		print_json_node(json, node);
	}

	ACL_FILE* fp = acl_fopen("./json.txt", "w");
	acl_assert(fp);
	ACL_VSTRING* buf = acl_json_build(json, NULL);
	acl_fwrite(acl_vstring_str(buf), ACL_VSTRING_LEN(buf), 1, fp);
	acl_fclose(fp);
	acl_vstring_free(buf);
	printf("\r\n");
}

static void test_json_foreach2(ACL_JSON* json)
{
	ACL_ITER iter1, iter2, iter3, iter4, iter5;
	ACL_JSON_NODE *node1, *node2, *node3, *node4, *node5;

	printf("------------ in %s ------------\r\n", __FUNCTION__);

	/* 一级结点 */
	acl_foreach(iter1, json->root) {
		node1 = (ACL_JSON_NODE*) iter1.data;
		print_json_node(json, node1);

		/* 二级结点 */
		acl_foreach(iter2, node1) {
			node2 = (ACL_JSON_NODE*) iter2.data;
			print_json_node(json, node2);

			/* 三级结点 */
			acl_foreach(iter3, node2) {
				node3 = (ACL_JSON_NODE*) iter3.data;
				print_json_node(json, node3);

				/* 四级结点 */
				acl_foreach(iter4, node3) {
					node4 = (ACL_JSON_NODE*) iter4.data;
					print_json_node(json, node4);

					/* 五级结点 */
					acl_foreach(iter5, node4) {
						node5 = (ACL_JSON_NODE*) iter5.data;
						print_json_node(json, node5);
					}
				}
			}
		}
	}
	printf("\r\n");
}

static void test_json_find1(ACL_JSON* json)
{
	printf("------------ in %s ------------\r\n", __FUNCTION__);

	//const char* tags = "menu/*/menuitem/*/onclick";
	const char* tags = "menu/popup/menuitem/*/onclick";
	//const char* tags = "menu/popup/menuitem";
	//const char* tags = "menu/popup/menuname";
	//const char* tags = "menu/id";

	printf(">>find: %s\r\n", tags);

	ACL_ARRAY* a = acl_json_getElementsByTags(json, tags);
	if (a == NULL) {
		printf("\r\n");
		return;
	}

	ACL_ITER iter1, iter2, iter3;
	acl_foreach(iter1, a) {
		ACL_JSON_NODE* node1 = (ACL_JSON_NODE*) iter1.data;
		printf("%s: %s\r\n", tags, STR(node1->text));

		/* 遍历 node1 结点的一级子结点 */
		acl_foreach(iter2, node1) {
			ACL_JSON_NODE* node2 = (ACL_JSON_NODE*) iter2.data;

			/* 遍历 node2 结点的一级子结点 */
			acl_foreach(iter3, node2) {
				ACL_JSON_NODE* node3 =
					(ACL_JSON_NODE*) iter3.data;
				printf("\t%s: %s\r\n", STR(node3->ltag),
						STR(node3->text));
			}
			printf("---------------------------------------\r\n");
		}
	}
	acl_json_free_array(a);
	printf(">>find %s end\r\n\r\n", tags);
}

static void test_json_find2(ACL_JSON* json)
{
	const char* tag = "onclick";
	ACL_ARRAY* a = acl_json_getElementsByTagName(json, tag);
	ACL_ITER iter;

	printf("------------ in %s ------------\r\n", __FUNCTION__);

	if (a) {
		acl_foreach(iter, a) {
			ACL_JSON_NODE* node = (ACL_JSON_NODE*) iter.data;
			printf("find %s result: %s\r\n", tag, STR(node->text));
		}
		acl_json_free_array(a);
	}

	tag = "help";
	a = acl_json_getElementsByTagName(json, tag);
	if (a) {
		acl_foreach(iter, a) {
			ACL_JSON_NODE* node = (ACL_JSON_NODE*) iter.data;
			printf("find %s result: %s\r\n", tag, STR(node->text));
		}
		acl_json_free_array(a);
	}
	printf(">>find %s end\r\n\r\n", tag);
}

static void test_json_data(const char* data)
{
	ACL_JSON* json = acl_json_alloc();
	const char* ptr = data;
	char  buf[2];
	ACL_VSTRING *tmp;

	json->flag |= ACL_JSON_FLAG_PART_WORD;

	if (1) {
		while (*ptr)
		{
			buf[0] = *ptr++;
			buf[1] = 0;
			acl_json_update(json, buf);
			/*
			if (json->finish)
				break;
				*/
		}
	}
	else
		acl_json_update(json, data);

	test_json_foreach1(json);
	test_json_foreach2(json);
	test_json_find1(json);
	test_json_find2(json);

	tmp = acl_vstring_alloc(128);
	acl_json_build(json, tmp);
	printf(">>>source: |%s|\r\n", data);
	printf(">>>result: |%s|\r\n", acl_vstring_str(tmp));
	acl_vstring_free(tmp);
	acl_json_free(json);
}

static void test_json_build(void)
{
	ACL_JSON* json = acl_json_alloc();
	ACL_JSON_NODE* root, *node1, *node2, *node3, *node4, *node5;

	root  = acl_json_create_obj(json);
	acl_json_node_append_child(json->root, root);

	node1 = acl_json_create_leaf(json, "name1", "value1");
	acl_json_node_append_child(root, node1);

	node1 = acl_json_create_leaf(json, "name2", "value2");
	acl_json_node_append_child(root, node1);

	node4 = acl_json_create_bool(json, "VAR_BOOL", 1);
	acl_json_node_append_child(root, node4);

	node5 = acl_json_create_null(json, "VAR_NULL");
	acl_json_node_append_child(root, node5);

	node1 = acl_json_create_obj(json);
	node2 = acl_json_create_leaf(json, "name3", "value3");
	acl_json_node_append_child(node1, node2);
	node2 = acl_json_create_node(json, "name4", node1);
	acl_json_node_append_child(root, node2);

	//////////////////////////////////////////////////////////////////////

	node1 = acl_json_create_array(json);
	node2 = acl_json_create_node(json, "name5", node1);
	acl_json_node_append_child(root, node2);

	node3 = acl_json_create_leaf(json, "name6", "value6");
	acl_json_node_append_child(node1, node3);

	node3 = acl_json_create_leaf(json, "name7", "value7");
	acl_json_node_append_child(node1, node3);

	node3 = acl_json_create_obj(json);
	acl_json_node_append_child(node1, node3);
	node2 = acl_json_create_leaf(json, "name8", "value8");
	acl_json_node_append_child(node3, node2);
	node2 = acl_json_create_leaf(json, "name9", "value9");
	acl_json_node_append_child(node3, node2);

	node4 = acl_json_create_array_null(json);
	acl_json_node_append_child(node1, node4);

	node5 = acl_json_create_array_bool(json, 1);
	acl_json_node_append_child(node1, node5);

	//////////////////////////////////////////////////////////////////////

	ACL_VSTRING* buf = acl_json_build(json, NULL);

	printf("--------------- after build --------------------------\r\n");
	printf("%s\r\n", acl_vstring_str(buf));
	acl_vstring_free(buf);

	acl_json_free(json);
}

static void test_json_default(void)
{
	test_json_data(default_data);
	test_json_build();
}

static void test_json_file(const char* path)
{
	char* buf = acl_vstream_loadfile(path);
	if (buf == NULL)
	{
		printf("load file %s error(%s)\r\n", path, acl_last_serror());
		return;
	}

	printf("buf: |%s|\r\n", buf);
	test_json_data(buf);
	acl_myfree(buf);
}

static void test_json_benchmark(bool once, int max)
{
	ACL_JSON *json = acl_json_alloc();

	ACL_METER_TIME("-------------bat begin--------------");

	for (int i = 0; i < max; i++)
	{
		const char* ptr = default_data;

		if (once)
			acl_json_update(json, ptr);
		else
		{
			/* 每次仅输入一个字节来分析 json 数据 */
			while (*ptr != 0) {
				char  ch2[2];

				ch2[0] = *ptr;
				ch2[1] = 0;
				acl_json_update(json, ch2);
				ptr++;
			}
		}
		acl_json_reset(json);
	}

	ACL_METER_TIME("-------------bat end--------------");
	acl_json_free(json);
}

static void usage(const char* program)
{
	printf("usage: %s -h[help]\n"
		" -f json_filepath\n"
		" -b[benchmark] -m benchmark_max\n"
		" -s[once parse]\n"
		" -M[use mempool]\r\n", program);
}

int main(int argc, char** argv)
{
#if 0
//	const char* pp = "誠\";
	const char* pp = "錦\";
	while (*pp)
	{
		printf("ch: %d\r\n", *pp);
		pp++;
	}
	printf("'\\': %d\r\n", '\\');
	exit(0);
#endif

	int   ch;
	int   benchmark_max = 100;
	bool  use_default = true, benchmark = false;
	bool  once = false, use_mempool = false;

	while ((ch = getopt(argc, argv, "hf:bm:sM")) > 0)
	{
		switch (ch)
		{
		case 'h':
			use_default = false;
			usage(argv[0]);
			return (0);
		case 'f':
			use_default = false;
			test_json_file(optarg);
			break;
		case 'b':
			use_default = false;
			benchmark = true;
			break;
		case 'm':
			use_default = false;
			benchmark_max = atoi(optarg);
			break;
		case 's':
			once = true;
			break;
		case 'M':
			use_mempool = true;
			break;
		default:
			break;
		}
	}

	if (use_mempool)
	{
		acl_mem_slice_init(8, 1024, 100000,
			ACL_SLICE_FLAG_GC2 |
			ACL_SLICE_FLAG_RTGC_OFF |
			ACL_SLICE_FLAG_LP64_ALIGN);
		printf("use mempool now\n");
	}

	if (benchmark)
		test_json_benchmark(once, benchmark_max);
	else if (use_default)
		test_json_default();

#ifdef	WIN32
	getchar();
#endif

	return (0);
}
