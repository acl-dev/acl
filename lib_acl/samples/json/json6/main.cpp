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
    "}\r\n"
    "{ 'hello world' }\r\n";


static void test_json(void)
{
	ACL_JSON *json = acl_json_alloc();
	const char* ptr = default_data;

	ptr = acl_json_update(json, ptr);
	printf("finish: %s, left char: %s\r\n",
		acl_json_finish(json) ? "yes" : "no", ptr);
	acl_json_free(json);
}

int main(void)
{
	test_json();

#ifdef	WIN32
	getchar();
#endif

	return (0);
}
