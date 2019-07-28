#include "lib_acl.h"

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN

#if 0

static const char *__json_string =
"{\r\n"
"  id: 'file', value: 'File',\r\n"
"  popup: {\r\n"
"    menuitem: [\r\n"
"      { value: 'New', onclick: 'CreateNewDoc()' },\r\n"
"      { value: 'Open', onclick: 'OpenDoc()' },\r\n"
"      { value: 'Close', onclick: 'CloseDoc()' }\r\n"
"    ],\r\n"
"    menuitem: 'help'\r\n"
"  },\r\n"
"  help: 'hello world!'\r\n"
"}\r\n";

#else

static const char *__json_string =
"{\"header\": {\"cmd\": \"8201\", \"tel\": \"011031000026\"}, \"body\" :{}}";

#endif

static void parse_json(const char *data)
{
	ACL_JSON *json = acl_json_alloc();
	ACL_VSTRING *buf1 = acl_vstring_alloc(128);
	ACL_VSTRING *buf2 = acl_vstring_alloc(128);
	ACL_ARRAY *nodes;
	const char *tag = "header";

	printf("buf    src: %s\r\n", data);

	printf("------------------------------------------------\r\n");

	acl_json_update(json, data);
	acl_json_build(json, buf1);
	printf("result src: %s\r\n", STR(buf1));

	printf("------------------------------------------------\r\n");

	nodes = acl_json_getElementsByTagName(json, tag);
	if (nodes == NULL)
	{
		printf("not found tag: %s\r\n", tag);
		acl_vstring_free(buf1);
		acl_vstring_free(buf2);
		acl_json_free(json);
		return;
	}

	printf(">>>tag: %s, len: %d\r\n", tag, acl_array_size(nodes));

	ACL_ITER iter;

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN

	acl_foreach(iter, nodes)
	{
		ACL_JSON_NODE *node = (ACL_JSON_NODE*) iter.data;
		ACL_JSON_NODE *tag_node = node->tag_node;
		if (tag_node == NULL)
			continue;

		printf(">>>tag: %s\r\n", STR(node->ltag));

		ACL_ITER iter2;
		acl_foreach(iter2, tag_node)
		{
			ACL_JSON_NODE *node1 = (ACL_JSON_NODE*) iter2.data;
			if (node1->ltag == NULL || LEN(node1->ltag) == 0)
				continue;
			printf(">>>child tag: %s, txt: %s\r\n", STR(node1->ltag),
				node1->text ? STR(node1->text) : "null");
		}
	}

	if (nodes)
		acl_json_free_array(nodes);

	acl_json_free(json);
	acl_vstring_free(buf1);
	acl_vstring_free(buf2);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch;
	char  filepath[256];

	filepath[0] = 0;

	while ((ch = getopt(argc, argv, "h")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}

	if (filepath[0]) {
		char *data = acl_vstream_loadfile(filepath);
		if (data) {
			parse_json(data);
			acl_myfree(data);
		}
	} else
		parse_json(__json_string);

	return 0;
}
