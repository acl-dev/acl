#include "lib_acl.h"

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN

#if 0
static const char *__json_string =
"{ menu: {\r\n"
"    id: 'file',\r\n"
"    value: 'File',\r\n"
"    popup: {\r\n"
"      menuitem: [\r\n"
"        { value: 'New', onclick: 'CreateNewDoc()' },\r\n"
"        { value: 'Open', onclick: 'OpenDoc()' },\r\n"
"        { value: 'Close', onclick: 'CloseDoc()' }\r\n"
"      ],\r\n"
"      menuitem: 'hello world'\r\n"
"    }\r\n"
"  }\r\n"
"}\r\n";
#endif

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

static void parse_json(const char *data)
{
	ACL_JSON *json = acl_json_alloc();
	ACL_JSON *json_new;
	ACL_VSTRING *buf1 = acl_vstring_alloc(128);
	ACL_VSTRING *buf2 = acl_vstring_alloc(128);
	ACL_ARRAY *nodes;
	ACL_JSON_NODE *one_node;
	const char *tags;

	printf("buf src: %s\r\n", data);

	printf("------------------------------------------------\r\n");

	json->flag |= ACL_JSON_FLAG_PART_WORD;
	acl_json_update(json, data);
	acl_json_build(json, buf1);
	printf("buf src: %s\r\n", STR(buf1));

	printf("------------------------------------------------\r\n");

	tags = "popup/menuitem";
	if (1)
		tags = "popup";
	else if (1)
		tags = "help";
	else
		tags = "popup/menuitem/*/onclick";

	nodes = acl_json_getElementsByTags(json, tags);
	if (nodes)
		one_node = (ACL_JSON_NODE*) acl_array_index(nodes, 0);
	else
		one_node = NULL;

	printf(">>>tags: %s\r\n", tags);
	if (one_node == NULL)
		json_new = acl_json_create(json->root);
	else
		json_new = acl_json_create(one_node);

	json_new->flag |= ACL_JSON_FLAG_PART_WORD;

	acl_json_build(json_new, buf2);
	printf("buf dst: %s\r\n", STR(buf2));

	printf("------------------------------------------------\r\n");

	if (nodes == NULL && strcmp(STR(buf1), STR(buf2)) != 0)
		printf("ERROR, not equal!\r\n");
	else
		printf("OK\r\n");

	if (nodes)
		acl_json_free_array(nodes);

	acl_json_free(json);
	acl_json_free(json_new);
	acl_vstring_free(buf1);
	acl_vstring_free(buf2);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -f json_filepath\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch;
	char  filepath[256];

	filepath[0] = 0;

	while ((ch = getopt(argc, argv, "hf:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			snprintf(filepath, sizeof(filepath), "%s", optarg);
			break;
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
