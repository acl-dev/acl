#include "lib_acl.h"

#define STR	acl_vstring_str

static const char* default_data = \
    "{ 'menu name': {\r\n"
    "    'id:file': 'file',\r\n"
    "    'value{': 'File',\r\n"
    "    'popup{}': {\r\n"
    "        'menuitem1}': [\r\n"
    "            {'value': 'New1', 'onclick': 'CreateNewDoc()'},\r\n"
    "            {'value': 'Open1', 'onclick': 'OpenDoc()'},\r\n"
    "            {'value': 'Close1', 'onclick': 'CloseDoc()'}\r\n"
    "        ],\r\n"
    "        'menuname[]': 'hello world',\r\n"
    "        'inner': { 'value' : 'new2 ', 'value' : 'open2' },\r\n"
    "        'menuitem2': [\r\n"
    "            {'value': 'New3', 'onclick': 'CreateNewDoc()'},\r\n"
    "            {'value': 'Open3', 'onclick': 'OpenDoc()'},\r\n"
    "            {'value': 'Close3', 'onclick': 'CloseDoc()'},\r\n"
    "            {{'value': 'Help3', 'onclick': 'Help()'}}"
    "        ]\r\n"
    "    }\r\n"
    " }\r\n,"
    " 'help': 'hello world!',\r\n"
    " 'menuitem2': [\r\n"
    "   {'value': 'New4', 'onclick': 'CreateNewDoc()'},\r\n"
    "   {'value': 'Open4', 'onclick': 'OpenDoc()'},\r\n"
    "   {'value': 'Close4', 'onclick': 'CloseDoc()'},\r\n"
    "   [{'value': 'Save4', 'onclick': 'SaveDoc()'}]"
    " ]\r\n"
    "}\r\n"
    "{ 'hello world' }\r\n";


static void test_json(void)
{
	ACL_JSON *json = acl_json_alloc();
	const char* ptr = default_data;
	ACL_VSTRING *buf = acl_vstring_alloc(1024);

	ptr = acl_json_update(json, ptr);

	printf("-------------------------------------------------------\r\n");
	printf("finish: %s, left char: %s\r\n",
		acl_json_finish(json) ? "yes" : "no", ptr);
	printf("-------------------------------------------------------\r\n");
	printf("\r\n%s\r\n", default_data);
	printf("-------------------------------------------------------\r\n");
	printf("\r\n%s\r\n", acl_vstring_str(buf));
	printf("-------------------------------------------------------\r\n");

	ACL_ARRAY *a = acl_json_getElementsByTags(json, "menuitem2/value");
	if (a) {
		ACL_ITER iter;

		acl_foreach(iter, a) {
			ACL_JSON_NODE* node = (ACL_JSON_NODE*) iter.data;
			printf("tag=%s, value=%s\r\n",
				acl_vstring_str(node->ltag),
				acl_vstring_str(node->text));
		}

		acl_json_free_array(a);
	}

	acl_json_build(json, buf);
	acl_json_free(json);
	acl_vstring_free(buf);
}

int main(void)
{
	test_json();

#ifdef	WIN32
	getchar();
#endif

	return (0);
}
