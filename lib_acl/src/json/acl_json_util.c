#include "StdAfx.h"
#include <stdio.h>
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_msg.h"
#include "json/acl_json.h"
#endif

#define STR	acl_vstring_str

void acl_json_free_array(ACL_ARRAY *a)
{
	acl_array_destroy(a, NULL);
}

ACL_ARRAY *acl_json_getElementsByTagName(ACL_JSON *json, const char *tag)
{
	ACL_ITER iter;
	ACL_ARRAY *a = acl_array_create(10);

	acl_foreach(iter, json) {
		ACL_JSON_NODE *node = (ACL_JSON_NODE*) iter.data;
		if (strcasecmp(tag, STR(node->ltag)) == 0) {
			acl_array_append(a, node);
		}
	}

	if (acl_array_size(a) == 0) {
		acl_array_destroy(a, NULL);
		return (NULL);
	}

	return (a);
}

ACL_ARRAY *acl_json_getElementsByTags(ACL_JSON *json, const char *tags)
{
	ACL_ARGV *tokens = acl_argv_split(tags, "/");
	ACL_ARRAY *a, *ret;
	ACL_ITER iter;

	a = acl_json_getElementsByTagName(json, tokens->argv[tokens->argc - 1]);
	if (a == NULL) {
		acl_argv_free(tokens);
		return (NULL);
	}

	ret = acl_array_create(acl_array_size(a));

	acl_foreach(iter, a) {
		ACL_JSON_NODE *node_saved, *node = (ACL_JSON_NODE*) iter.data;
		int   i = tokens->argc - 1;
		node_saved = node;
		while (i >= 0 && node->parent != NULL) {
			if (strcasecmp(tokens->argv[i], "*") != 0 &&
				strcasecmp(tokens->argv[i], STR(node->ltag)) != 0)
			{
				break;
			}
			i--;
			node = node->parent;
		}
		if (i == -1)
			ret->push_back(ret, node_saved);
	}

	acl_json_free_array(a);
	acl_argv_free(tokens);

	if (acl_array_size(ret) == 0) {
		acl_array_free(ret, NULL);
		ret = NULL;
	}
	return (ret);
}

#define LEN	ACL_VSTRING_LEN
#define STR	acl_vstring_str

ACL_JSON_NODE *acl_json_create_leaf(ACL_JSON *json,
	const char *name, const char *text)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->ltag, name);
	acl_vstring_strcpy(node->text, text);
	node->type = ACL_JSON_T_LEAF;
	return (node);
}

ACL_JSON_NODE *acl_json_create_obj(ACL_JSON *json)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	node->left_ch = '{';
	node->right_ch = '}';
	node->type = ACL_JSON_T_OBJ;
	return (node);
}

ACL_JSON_NODE *acl_json_create_array(ACL_JSON *json)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	node->left_ch = '[';
	node->right_ch = ']';
	node->type = ACL_JSON_T_ARRAY;
	return (node);
}

ACL_JSON_NODE *acl_json_create_node(ACL_JSON *json,
	const char *name, ACL_JSON_NODE *value)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->ltag, name);
	node->tag_node = value;
	node->type = ACL_JSON_T_NODE;
	acl_json_node_add_child(node, value);
	return (node);
}

void acl_json_node_append_child(ACL_JSON_NODE *parent, ACL_JSON_NODE *child)
{
	const char *myname = "acl_json_node_append_child";

	if (parent->type != ACL_JSON_T_ARRAY
		&& parent->type != ACL_JSON_T_OBJ
		&& parent != parent->json->root)
	{
		acl_msg_fatal("%s(%d): parent's type not array or obj",
			myname, __LINE__);
	}
	acl_json_node_add_child(parent, child);
}

static void json_escape_append(ACL_VSTRING *buf, const char *src)
{
	const unsigned char *ptr = (const unsigned char*) src;

	ACL_VSTRING_ADDCH(buf, '"');
	while (*ptr) {
		if (*ptr == '"' || *ptr == '\\' || *ptr == '/') {
			ACL_VSTRING_ADDCH(buf, '\\');
			ACL_VSTRING_ADDCH(buf, *ptr);
		} else if (*ptr == '\b') {
			ACL_VSTRING_ADDCH(buf, '\\');
			ACL_VSTRING_ADDCH(buf, 'b');
		} else if (*ptr == '\f') {
			ACL_VSTRING_ADDCH(buf, '\\');
			ACL_VSTRING_ADDCH(buf, 'f');
		} else if (*ptr == '\n') {
			ACL_VSTRING_ADDCH(buf, '\\');
			ACL_VSTRING_ADDCH(buf, 'n');
		} else if (*ptr == '\r') {
			ACL_VSTRING_ADDCH(buf, '\\');
			ACL_VSTRING_ADDCH(buf, 'r');
		} else if (*ptr == '\t') {
			ACL_VSTRING_ADDCH(buf, '\\');
			ACL_VSTRING_ADDCH(buf, 't');
		} else
			ACL_VSTRING_ADDCH(buf, *ptr);
		ptr++;
	}
	ACL_VSTRING_ADDCH(buf, '"');
	ACL_VSTRING_TERMINATE(buf);
}

ACL_VSTRING *acl_json_build(ACL_JSON *json, ACL_VSTRING *buf)
{
	ACL_JSON_NODE *node, *prev;
	ACL_ITER iter;

	if (buf == NULL)
		buf = acl_vstring_alloc(256);

	if (json->root->left_ch > 0) {
		ACL_VSTRING_ADDCH(buf, json->root->left_ch);
		ACL_VSTRING_TERMINATE(buf);
	}

	acl_foreach(iter, json) {
		node = (ACL_JSON_NODE*) iter.data;
		prev = acl_json_node_prev(node);

		/* 只有当标签的对应值为 JSON 对象或数组对象时 tag_node 非空 */
		if (node->tag_node != NULL) {
			if (prev != NULL)
				acl_vstring_strcat(buf, ", ");
			json_escape_append(buf, STR(node->ltag));
			ACL_VSTRING_ADDCH(buf, ':');
			ACL_VSTRING_ADDCH(buf, ' ');
			if (node->left_ch != 0) {
				/* '{' or '[' */
				ACL_VSTRING_ADDCH(buf, node->left_ch);
			}
			ACL_VSTRING_TERMINATE(buf);
			continue;
		}

		if (prev)
			acl_vstring_strcat(buf, ", ");

		/* 当结点有标签名时 */
		if (LEN(node->ltag) > 0) {
			json_escape_append(buf, STR(node->ltag));
			ACL_VSTRING_ADDCH(buf, ':');
			ACL_VSTRING_ADDCH(buf, ' ');
			json_escape_append(buf, STR(node->text));
		}

		/* 当结点为没有标签名的容器(为 '{}' 或 '[]')时 */
		else if (node->left_ch != 0) {
			ACL_VSTRING_ADDCH(buf, node->left_ch);
			ACL_VSTRING_TERMINATE(buf);
		}

		/* 当本结点有子结点或虽为叶结点，但该结点的下一个兄弟结点
		 * 非空时继续下一个循环过程
		 */
		if (acl_json_node_next(node) != NULL || acl_ring_size(&node->children) > 0)
			continue;

		/* 当本结点为叶结点且后面没有兄弟结点时，需要一级一级回溯将父结点的分隔符
		 * 添加至本叶结点尾部，直到遇到根结点或父结点的下一个兄弟结点非空
		 */
		while (node != node->json->root && acl_json_node_next(node) == NULL) {
			if (node->parent->right_ch != 0) {
				/* node->parent->right_ch: '}' or ']' */
				ACL_VSTRING_ADDCH(buf, node->parent->right_ch);
				ACL_VSTRING_TERMINATE(buf);
			}
			node = node->parent;
		}
	}
	return (buf);
}
