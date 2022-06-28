#include "lib_acl.h"

#define	LEN	ACL_VSTRING_LEN
#define STR	acl_vstring_str

static int build_callback(ACL_JSON *, ACL_VSTRING *data, void *ctx)
{
	ACL_VSTRING *buf = (ACL_VSTRING*) ctx;

	if (data == NULL) {
		printf("ALL OVER NOW!\r\n");
		return 0;
	}

	printf("input: length: %d, |%s|\r\n", (int) LEN(data), STR(data));
	acl_vstring_strcat(buf, STR(data));
	return 0;
}

static void test_json_build(void)
{
	ACL_JSON* json = acl_json_alloc();
	ACL_JSON_NODE* root, *node1, *node2, *node3, *node4;

	root  = acl_json_create_obj(json);
	acl_json_node_append_child(json->root, root);

	node1 = acl_json_create_leaf(json, "name1", "value1");
	acl_json_node_append_child(root, node1);

	node1 = acl_json_create_leaf(json, "name2", "value2");
	acl_json_node_append_child(root, node1);

	node1 = acl_json_create_obj(json);
	node2 = acl_json_create_leaf(json, "name3", "value3");
	acl_json_node_append_child(node1, node2);
	node2 = acl_json_create_node(json, "name4", node1);
	acl_json_node_append_child(root, node2);

	node3 = acl_json_create_bool(json, "VAR_BOOL", 0);
	acl_json_node_append_child(root, node3);

	node4 = acl_json_create_null(json, "VAR_NULL");
	acl_json_node_append_child(root, node4);

	//////////////////////////////////////////////////////////////////////////

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

	node4 = acl_json_create_array_bool(json, 1);
	acl_json_node_append_child(node1, node4);

	node4 = acl_json_create_array_null(json);
	acl_json_node_append_child(node1, node4);

	//////////////////////////////////////////////////////////////////////////

	ACL_VSTRING *buf1 = acl_vstring_alloc(128);
	ACL_VSTRING *buf2 = acl_vstring_alloc(128);

	acl_json_build(json, buf1);
	acl_json_building(json, 1, build_callback, buf2);

	printf("%s\r\n", acl_vstring_str(buf1));
	printf("%s\r\n", acl_vstring_str(buf2));

	if (strcmp(STR(buf1), STR(buf2)) != 0)
		printf("BUILD ERROR\r\n");
	else
		printf("BUILD OK\r\n");

	acl_vstring_free(buf1);
	acl_vstring_free(buf2);

	acl_json_free(json);
}

int main(void)
{
	test_json_build();

#ifdef	WIN32
	getchar();
#endif

	return (0);
}
