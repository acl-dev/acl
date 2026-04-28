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
#define	LEN	ACL_VSTRING_LEN

ACL_JSON_NODE *acl_json_getFirstElementByTagName(
	ACL_JSON *json, const char *tag)
{
	ACL_ITER iter;

	acl_foreach(iter, json) {
		ACL_JSON_NODE *node = (ACL_JSON_NODE*) iter.data;
		if (strcasecmp(tag, STR(node->ltag)) == 0) {
			return node;
		}
	}

	return NULL;
}

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
		return NULL;
	}

	return a;
}

ACL_ARRAY *acl_json_getElementsByTags(ACL_JSON *json, const char *tags)
{
	ACL_ARGV *tokens = acl_argv_split(tags, "/");
	ACL_ARRAY *a, *result;
	ACL_ITER iter;
	ACL_JSON_NODE *node_saved, *node;
	int   i;

	a = acl_json_getElementsByTagName(json, tokens->argv[tokens->argc - 1]);
	if (a == NULL) {
		acl_argv_free(tokens);
		return NULL;
	}

	result = acl_array_create(acl_array_size(a));

#define	NEQ(x, y) strcasecmp((x), (y))

	acl_foreach(iter, a) {
		node = (ACL_JSON_NODE*) iter.data;
		node_saved = node;
		i = tokens->argc - 1;
		while (i >= 0 && node->parent != NULL) {
			if (node->left_ch != 0) {
				node = node->parent;
			} else if (NEQ(tokens->argv[i], "*")
				&& NEQ(tokens->argv[i], STR(node->ltag))) {

				break;
			} else {
				i--;
				node = node->parent;
			}
		}
		if (i == -1) {
			result->push_back(result, node_saved);
		}
	}

	acl_json_free_array(a);
	acl_argv_free(tokens);

	if (acl_array_size(result) == 0) {
		acl_array_free(result, NULL);
		result = NULL;
	}
	return result;
}

#define LEN	ACL_VSTRING_LEN
#define STR	acl_vstring_str

ACL_JSON_NODE *acl_json_create_text(ACL_JSON *json,
	const char *name, const char *value)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->ltag, name);
	acl_vstring_strcpy(node->text, value);
	node->type = ACL_JSON_T_STRING;
	return node;
}

ACL_JSON_NODE *acl_json_create_bool(ACL_JSON *json,
	const char *name, int value)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->ltag, name);
	acl_vstring_strcpy(node->text, value == 0 ? "false" : "true");
	node->type = ACL_JSON_T_BOOL;
	return node;
}

ACL_JSON_NODE *acl_json_create_null(ACL_JSON *json, const char *name)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->ltag, name);
	acl_vstring_strcpy(node->text, "null");
	node->type = ACL_JSON_T_NULL;
	return node;
}

ACL_JSON_NODE *acl_json_create_int64(ACL_JSON *json,
	const char *name, acl_int64 value)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->ltag, name);
	acl_vstring_sprintf(node->text, "%lld", value);
	node->type = ACL_JSON_T_NUMBER;
	return node;
}

ACL_JSON_NODE *acl_json_create_double(ACL_JSON *json,
	const char *name, double value)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->ltag, name);
	acl_vstring_sprintf(node->text, "%.4f", value);
	node->type = ACL_JSON_T_DOUBLE;
	return node;
}

ACL_JSON_NODE *acl_json_create_double2(ACL_JSON *json,
	const char *name, double value, int precision)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);
	char fmt[16];

	acl_vstring_strcpy(node->ltag, name);
	if (precision <= 0) {
		precision = 4;
	}

	snprintf(fmt, sizeof(fmt), "%%.%df", precision);
	acl_vstring_sprintf(node->text, fmt, value);
	node->type = ACL_JSON_T_DOUBLE;
	return node;
}

ACL_JSON_NODE *acl_json_create_array_text(ACL_JSON *json, const char *text)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->text, text);
	node->type = ACL_JSON_T_A_STRING;
	return node;
}

ACL_JSON_NODE *acl_json_create_array_int64(ACL_JSON *json, acl_int64 value)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_sprintf(node->text, "%lld", value);
	node->type = ACL_JSON_T_A_NUMBER;
	return node;
}

ACL_JSON_NODE *acl_json_create_array_double(ACL_JSON *json, double value)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_sprintf(node->text, "%.4f", value);
	node->type = ACL_JSON_T_A_DOUBLE;
	return node;
}

ACL_JSON_NODE *acl_json_create_array_bool(ACL_JSON *json, int value)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->text, value ? "true" : "false");
	node->type = ACL_JSON_T_A_BOOL;
	return node;
}

ACL_JSON_NODE *acl_json_create_array_null(ACL_JSON *json)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->text, "null");
	node->type = ACL_JSON_T_A_NULL;
	return node;
}

ACL_JSON_NODE *acl_json_create_obj(ACL_JSON *json)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	node->left_ch = '{';
	node->right_ch = '}';
	node->type = ACL_JSON_T_OBJ;
	return node;
}

ACL_JSON_NODE *acl_json_create_array(ACL_JSON *json)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	node->left_ch = '[';
	node->right_ch = ']';
	node->type = ACL_JSON_T_ARRAY;
	return node;
}

ACL_JSON_NODE *acl_json_create_node(ACL_JSON *json,
	const char *name, ACL_JSON_NODE *value)
{
	ACL_JSON_NODE *node = acl_json_node_alloc(json);

	acl_vstring_strcpy(node->ltag, name);
	node->tag_node = value;
	node->type = ACL_JSON_T_OBJ;
	acl_json_node_add_child(node, value);
	return node;
}

void acl_json_node_append_child(ACL_JSON_NODE *parent, ACL_JSON_NODE *child)
{
	const char *myname = "acl_json_node_append_child";

	if (parent->type != ACL_JSON_T_ARRAY
	    && parent->type != ACL_JSON_T_OBJ
	    && parent != parent->json->root) {
		acl_msg_fatal("%s(%d): parent's type not array or obj",
			myname, __LINE__);
	}
	acl_json_node_add_child(parent, child);
}

static void json_escape_append(ACL_VSTRING *buf, const char *src)
{
	const unsigned char *ptr = (const unsigned char*) src;
	char tmp[8];

	ACL_VSTRING_ADDCH(buf, '"');

	while (*ptr) {
		if (*ptr == '"' || *ptr == '\\') {
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
		} else if (*ptr < 0x20) {
			/* 其余 C0 控制字符（0x00–0x1F）输出 \uXXXX */
			snprintf(tmp, sizeof(tmp), "\\u%04X", (unsigned)*ptr);
			acl_vstring_strcat(buf, tmp);
		} else if (*ptr == 0xE2
			&& *(ptr + 1) == 0x80
			&& (*(ptr + 2) == 0xA8 || *(ptr + 2) == 0xA9)) {
			/* UTF-8 编码的 U+2028 行分隔符和 U+2029 段分隔符，
			 * 在 JSON 字符串中需要转义以确保嵌入 JavaScript 时安全 */
			snprintf(tmp, sizeof(tmp), "\\u20%s",
				*(ptr + 2) == 0xA8 ? "28" : "29");
			acl_vstring_strcat(buf, tmp);
			ptr += 2; /* 跳过剩余两个字节，ptr++ 在最后执行 */
		} else
			ACL_VSTRING_ADDCH(buf, *ptr);
		ptr++;
	}
	ACL_VSTRING_ADDCH(buf, '"');
	ACL_VSTRING_TERMINATE(buf);
}

static void child_end(ACL_JSON *json, ACL_JSON_NODE *node, ACL_VSTRING *buf)
{
	/* 当本节点为叶节点且后面没有兄弟节点时，需要一级一级回溯
	 * 将父节点的分隔符添加至本叶节点尾部，直到遇到根节点或父
	 * 节点的下一个兄弟节点非空
	 */
	while (acl_json_node_next(node) == NULL) {
		if (node->parent == json->root) {
			break;
		}

		node = node->parent;

		/* right_ch: '}' or ']' */
		if (node->right_ch != 0) {
			ACL_VSTRING_ADDCH(buf, node->right_ch);
		}
	}
}

void acl_json_building(ACL_JSON *json, size_t length,
	int (*callback)(ACL_JSON *, ACL_VSTRING *, void *), void *ctx)
{
	ACL_ITER iter;
	ACL_JSON_NODE *node, *prev;
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	ACL_RING *ring_ptr = acl_ring_succ(&json->root->children);

	/* 为了兼容历史的BUG，所以此处只能如此处理了--zsx, 2021.3.27 */

	if (ring_ptr == &json->root->children) {
		if (json->root->left_ch == 0) {
			json->root->left_ch = '{';
			json->root->right_ch = '}';
		}
	} else if (acl_ring_size(&json->root->children) == 1) {
		node = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
		if (node->left_ch == 0 && json->root->left_ch == 0) {
			json->root->left_ch = '{';
			json->root->right_ch = '}';
		}
	} else if (json->root->left_ch == 0) {
		json->root->left_ch = '{';
		json->root->right_ch = '}';
	}

	if (json->root->left_ch > 0) {
		ACL_VSTRING_ADDCH(buf, json->root->left_ch);
	}

	acl_foreach(iter, json) {
		if (ACL_VSTRING_LEN(buf) >= length && callback != NULL) {
			ACL_VSTRING_TERMINATE(buf);
			if (callback(json, buf, ctx) < 0) {
				acl_vstring_free(buf);
				return;
			}
			ACL_VSTRING_RESET(buf);
		}

		node = (ACL_JSON_NODE*) iter.data;
		prev = acl_json_node_prev(node);
		if (prev != NULL) {
			if ((json->flag & ACL_JSON_FLAG_ADD_SPACE)) {
				acl_vstring_strcat(buf, ", ");
			} else {
				acl_vstring_strcat(buf, ",");
			}
		}

		/* 只有当标签的对应值为 JSON 对象或数组对象时 tag_node 非空 */
		if (node->tag_node != NULL) {
			if (LEN(node->ltag) > 0) {
				json_escape_append(buf, STR(node->ltag));
				ACL_VSTRING_ADDCH(buf, ':');
				if ((json->flag & ACL_JSON_FLAG_ADD_SPACE)) {
					ACL_VSTRING_ADDCH(buf, ' ');
				}
			}

			/* '{' or '[' */	
			if (node->left_ch != 0) {
				ACL_VSTRING_ADDCH(buf, node->left_ch);
			}
		}

		/* 当节点有标签名时 */
		else if (LEN(node->ltag) > 0) {
			json_escape_append(buf, STR(node->ltag));
			ACL_VSTRING_ADDCH(buf, ':');
			if ((json->flag & ACL_JSON_FLAG_ADD_SPACE)) {
				ACL_VSTRING_ADDCH(buf, ' ');
			}

			switch (node->type & ~ACL_JSON_T_LEAF) {
			case ACL_JSON_T_NULL:
				acl_vstring_strcat(buf, "null");
				break;
			case ACL_JSON_T_BOOL:
			case ACL_JSON_T_NUMBER:
			case ACL_JSON_T_DOUBLE:
				acl_vstring_strcat(buf, STR(node->text));
				break;
			default:
				json_escape_append(buf, STR(node->text));
				break;
			}
		}

		/* 当节点为数组的成员时 */
		else if (node->parent && node->parent->type == ACL_JSON_T_ARRAY
			 && (node->type & ACL_JSON_T_A_TYPES)) {
			switch (node->type & ~ACL_JSON_T_LEAF) {
			case ACL_JSON_T_A_NULL:
				acl_vstring_strcat(buf, "null");
				break;
			case ACL_JSON_T_A_BOOL:
			case ACL_JSON_T_A_NUMBER:
			case ACL_JSON_T_A_DOUBLE:
				acl_vstring_strcat(buf, STR(node->text));
				break;
			default:
				json_escape_append(buf, STR(node->text));
				break;
			}
		}

		/* 当节点为没有标签名的容器(为 '{}' 或 '[]')时 */
		else if (node->left_ch != 0) {
			ACL_VSTRING_ADDCH(buf, node->left_ch);
		}

		/*
		 * 遍历方式为前序遍历方式，即先遍历当前节点的子节点，
		 * 再遍历当前节点的子节点，最后遍历当前节点的父节点
		 */
		/* 当本节点有子节点或虽为叶节点，但该节点的下一个兄弟节点
		 * 非空时继续下一个循环过程
		 */
		if (acl_ring_size(&node->children) > 0) {
			continue;
		} else if (acl_json_node_next(node) != NULL) {
			if (node->right_ch > 0) {
				ACL_VSTRING_ADDCH(buf, node->right_ch);
			}
			continue;
		}

		if (node->right_ch > 0) {
			ACL_VSTRING_ADDCH(buf, node->right_ch);
		}

		/* 当本节点为叶节点且后面没有兄弟节点时，需要一级一级回溯
		 * 将父节点的分隔符添加至本叶节点尾部，直到遇到根节点或父
		 * 节点的下一个兄弟节点非空
		 */
		child_end(json, node, buf);
	}

	if (json->root->right_ch > 0) {
		ACL_VSTRING_ADDCH(buf, json->root->right_ch);
	}

	ACL_VSTRING_TERMINATE(buf);
	if (ACL_VSTRING_LEN(buf) > 0 && callback != NULL) {
		if (callback(json, buf, ctx) < 0) {
			acl_vstring_free(buf);
			return;
		}
	}

	acl_vstring_free(buf);

	/* 将第二个参数置 NULL 表示处理完毕 */
	if (callback != NULL) {
		(void) callback(json, NULL, ctx);
	}
}

static int is_parents_disabled(ACL_JSON_NODE *node)
{
	while ((node = node->parent)) {
		if (node->disabled) {
			return 1;
		}
	}
	return 0;
}

static int is_first_node(ACL_JSON_NODE *node)
{
	while ((node = acl_json_node_prev(node))) {
		if (!node->disabled) {
			return 0;
		}
	}
	return 1;
}

ACL_VSTRING *acl_json_build(ACL_JSON *json, ACL_VSTRING *buf)
{
	ACL_JSON_NODE *node, *prev;
	ACL_ITER iter;
	ACL_RING *ring_ptr = acl_ring_succ(&json->root->children);

	if (buf == NULL) {
		buf = acl_vstring_alloc(256);
	}

	/* 为了兼容历史的BUG，所以此处只能如此处理了--zsx, 2021.3.27 */

	if (ring_ptr == &json->root->children) {
		if (json->root->left_ch == 0) {
			json->root->left_ch = '{';
			json->root->right_ch = '}';
		}
	} else if (acl_ring_size(&json->root->children) == 1) {
		node = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
		if (node->left_ch == 0 && json->root->left_ch == 0) {
			json->root->left_ch = '{';
			json->root->right_ch = '}';
		}
	} else if (json->root->left_ch == 0) {
		json->root->left_ch = '{';
		json->root->right_ch = '}';
	}

	if (json->root->left_ch > 0) {
		ACL_VSTRING_ADDCH(buf, json->root->left_ch);
	}

	acl_foreach(iter, json) {
		node = (ACL_JSON_NODE*) iter.data;
		if (node->disabled) { // 跳过被禁止的 json 节点
			child_end(json, node, buf);
			continue;
		}

		if (is_parents_disabled(node)) {
			continue;
		}

		prev = acl_json_node_prev(node);
		if (prev != NULL && (!prev->disabled || !is_first_node(node))) {
			if ((json->flag & ACL_JSON_FLAG_ADD_SPACE)) {
				acl_vstring_strcat(buf, ", ");
			} else {
				acl_vstring_strcat(buf, ",");
			}
		}

		/* 只有当标签的对应值为 JSON 对象或数组对象时 tag_node 非空 */
		if (node->tag_node != NULL) {
			if (LEN(node->ltag) > 0) {
				json_escape_append(buf, STR(node->ltag));
				ACL_VSTRING_ADDCH(buf, ':');
				if ((json->flag & ACL_JSON_FLAG_ADD_SPACE)) {
					ACL_VSTRING_ADDCH(buf, ' ');
				}
			}

			/* '{' or '[' */	
			if (node->left_ch != 0) {
				ACL_VSTRING_ADDCH(buf, node->left_ch);
			}
		}

		/* 当节点有标签名时 */
		else if (LEN(node->ltag) > 0) {
			json_escape_append(buf, STR(node->ltag));
			ACL_VSTRING_ADDCH(buf, ':');
			if ((json->flag & ACL_JSON_FLAG_ADD_SPACE)) {
				ACL_VSTRING_ADDCH(buf, ' ');
			}

			switch (node->type & ~ACL_JSON_T_LEAF) {
			case ACL_JSON_T_NULL:
				acl_vstring_strcat(buf, "null");
				break;
			case ACL_JSON_T_BOOL:
			case ACL_JSON_T_NUMBER:
			case ACL_JSON_T_DOUBLE:
				acl_vstring_strcat(buf, STR(node->text));
				break;
			default:
				json_escape_append(buf, STR(node->text));
				break;
			}
		}

		/* 当节点为数组的成员时 */
		else if (node->parent && node->parent->type == ACL_JSON_T_ARRAY
			 && (node->type & ACL_JSON_T_A_TYPES)) {
			switch (node->type & ~ACL_JSON_T_LEAF) {
			case ACL_JSON_T_A_NULL:
				acl_vstring_strcat(buf, "null");
				break;
			case ACL_JSON_T_A_BOOL:
			case ACL_JSON_T_A_NUMBER:
			case ACL_JSON_T_A_DOUBLE:
				acl_vstring_strcat(buf, STR(node->text));
				break;
			case ACL_JSON_T_A_STRING:
				json_escape_append(buf, STR(node->text));
				break;
			default:
				break;
			}
		}

		/* 当节点为没有标签名的容器(为 '{}' 或 '[]')时 */
		else if (node->left_ch != 0) {
			ACL_VSTRING_ADDCH(buf, node->left_ch);
		}

		/*
		 * 遍历方式为前序遍历方式，即先遍历当前节点的子节点，
		 * 再遍历当前节点的子节点，最后遍历当前节点的父节点
		 */
		/* 当本节点有子节点或虽为叶节点，但该节点的下一个兄弟节点
		 * 非空时继续下一个循环过程
		 */
		if (acl_ring_size(&node->children) > 0) {
			continue;
		} else if (acl_json_node_next(node) != NULL) {
			if (node->right_ch > 0) {
				ACL_VSTRING_ADDCH(buf, node->right_ch);
			}
			continue;
		}

		if (node->right_ch > 0) {
			ACL_VSTRING_ADDCH(buf, node->right_ch);
		}

		/* 当本节点为叶节点且后面没有兄弟节点时，需要一级一级回溯
		 * 将父节点的分隔符添加至本叶节点尾部，直到遇到根节点或父
		 * 节点的下一个兄弟节点非空
		 */
		child_end(json, node, buf);
	}

	if (json->root->right_ch > 0) {
		ACL_VSTRING_ADDCH(buf, json->root->right_ch);
	}

	ACL_VSTRING_TERMINATE(buf);
	return buf;
}

ACL_VSTRING *acl_json_node_build(ACL_JSON_NODE *node, ACL_VSTRING *buf)
{
	ACL_JSON *json = acl_json_alloc();
	ACL_JSON_NODE *first;

	if (buf == NULL) {
		buf = acl_vstring_alloc(256);
	}

	if (node == node->json->root && node->tag_node != NULL) {
		node = node->tag_node;
	} else {
		json->root->left_ch = json->root->right_ch = 0;
	}

	first = acl_json_node_duplicate(json, node);
	acl_json_node_add_child(json->root, first);
	acl_json_build(json, buf);
	acl_json_free(json);

	return buf;
}
