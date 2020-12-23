#include "StdAfx.h"
#include <stdio.h>
#include <string.h>
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_define.h"
#include "stdlib/acl_stringops.h"
#include "json/acl_json.h"
#endif

#define	LEN	ACL_VSTRING_LEN
#define	STR	acl_vstring_str
#define END	acl_vstring_end
#define ADDCH	ACL_VSTRING_ADDCH

#define IS_QUOTE(x) ((x) == '\"' || (x) == '\'')
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#define SKIP_WHILE(cond, ptr) { while(*(ptr) && (cond)) (ptr)++; }
#define SKIP_SPACE(ptr) { while(IS_SPACE(*(ptr))) (ptr)++; }

static const char *json_root(ACL_JSON *json, const char *data)
{
	SKIP_WHILE(*data != '{' && *data != '[', data);
	if (*data == 0)
		return data;

	if (*data == '{') {
		json->root->left_ch = '{';
		json->root->right_ch = '}';
		json->status = ACL_JSON_S_MEMBER;
		json->root->type = ACL_JSON_T_OBJ;
	}
	else
	{
		json->root->left_ch = '[';
		json->root->right_ch = ']';
		json->status = ACL_JSON_S_ELEMENT;
		json->root->type = ACL_JSON_T_ARRAY;
	}

	data++;

	json->curr_node = json->root;
	json->depth = json->depth;

	return data;
}

/* 分析节点对象值，必须找到 '{' 或 '[' */

static const char *json_obj(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *obj;

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	/* 创建对象 '{}' 子节点 */

	obj = acl_json_node_alloc(json);
	obj->type = ACL_JSON_T_OBJ;
	obj->depth = json->curr_node->depth + 1;
	if (obj->depth > json->depth)
		json->depth = obj->depth;

	/* 根据 json 节点对象前缀的不同，记录不同的对象后缀 */
	obj->left_ch = '{';
	obj->right_ch = '}';

	acl_json_node_add_child(json->curr_node, obj);

	if (LEN(json->curr_node->ltag) > 0)
		json->curr_node->tag_node = obj;

	json->curr_node = obj;
	json->status = ACL_JSON_S_MEMBER;

	return data;
}

static const char *json_member(ACL_JSON *json, const char *data)
{
	/* 创建上面所建对象节点的成员对象 */
	ACL_JSON_NODE *member = acl_json_node_alloc(json);

	member->type = ACL_JSON_T_MEMBER;
	member->depth = json->curr_node->depth + 1;
	if (member->depth > json->depth)
		json->depth = member->depth;

	acl_json_node_add_child(json->curr_node, member);

	/* 将该成员对象置为当前 JSON 分析节点 */
	json->curr_node = member;
	json->status = ACL_JSON_S_PAIR;

	return data;
}

/* 解析节点的标签名称，节点允许没有标签名；叶节点没有 { } [ ] 分隔符 */

static const char *json_pair(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *parent = acl_json_node_parent(json->curr_node);

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	acl_assert(parent);

	/* 如果当前字符为父节点的右分隔符，则表示父节点结束 */
	if (*data == parent->right_ch) {
		data++;  /* 去掉父节点的右分隔符 */
		if (parent == json->root) {
			/* 如果根节点分析结束则整个 json 分析完毕 */
			json->finish = 1;
			return data;
		}
		/* 弹出父节点 */
		json->curr_node = parent;
		/* 查询父节点的下一个兄弟节点 */
		json->status = ACL_JSON_S_NEXT;
		return data;
	}

	/* 为 '{' 或 '[' 时说明遇到了当前节点的子节点 */
	if (*data == '{') {
		data++;
		json->status = ACL_JSON_S_OBJ;
		return data;
	}
	else if (*data == '[') {
		data++;
		json->status = ACL_JSON_S_ARRAY;
		return data;
	}

	/* 如果标签名前有引号，记录下该引号 */
	if (IS_QUOTE(*data) && json->curr_node->quote == 0)
		json->curr_node->quote = *data++;

	json->curr_node->type = ACL_JSON_T_PAIR;
	json->status = ACL_JSON_S_TAG;

	return data;
}

/* 解析节点的标签名称，节点允许没有标签名；叶节点没有 { } [ ] 分隔符 */

static const char *json_tag(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *node = json->curr_node;
	char ch;

	while ((ch = *data) != 0) {
		/* 如果前面有引号，则需要找到结尾引号 */
		if (node->quote) {
			if (node->backslash) {
				if (ch == 'b')
					ADDCH(node->ltag, '\b');
				else if (ch == 'f')
					ADDCH(node->ltag, '\f');
				else if (ch == 'n')
					ADDCH(node->ltag, '\n');
				else if (ch == 'r')
					ADDCH(node->ltag, '\r');
				else if (ch == 't')
					ADDCH(node->ltag, '\t');
				else
					ADDCH(node->ltag, ch);
				node->backslash = 0;
			}

			/* 当为双字节汉字时，第一个字节为的高位为 1，
			 * 第二个字节为 92，正好与转义字符相同
			 */
			else if (ch == '\\') {
				/* 处理半个汉字的情形 */
				if (node->part_word) {
					ADDCH(node->ltag, ch);
					node->part_word = 0;
				} else
					node->backslash = 1;
			} else if (ch == node->quote) {
				ACL_JSON_NODE *parent;

				parent = acl_json_node_parent(node);

				//acl_assert(parent);

				/* 数组对象的子节点允许为单独的字符串或对象 */
				if (parent->left_ch == '[')
					json->status = ACL_JSON_S_NEXT;

				/* 标签值分析结束，下一步需要找到冒号 */
				else
					json->status = ACL_JSON_S_COLON;

				/* 当在分析标签名结束后，需要把 quote 赋 0，
				 * 这样在分析标签值时，可以复用该 quote 变量,
				 * 如果不清 0，则会干扰分析标签值过程
				 */
				node->quote = 0;
				node->part_word = 0;
				data++;
				break;
			}

			/* 是否兼容后半个汉字为转义符 '\' 的情况 */
			else if ((json->flag & ACL_JSON_FLAG_PART_WORD)) {
				ADDCH(node->ltag, ch);

				/* 处理半个汉字的情形 */
				if (node->part_word)
					node->part_word = 0;
				else if (ch < 0)
					node->part_word = 1;
			} else {
				ADDCH(node->ltag, ch);
			}
		}

		/* 分析标签名前没有引号的情况 */

		else if (node->backslash) {
			ADDCH(node->ltag, ch);
			node->backslash = 0;
		}

		/* 当为双字节汉字时，第一个字节为的高位为 1，
		 * 第二个字节为 92，正好与转义字符相同
		 */
		else if (ch == '\\') {
			/* 处理半个汉字的情形 */
			if (node->part_word) {
				ADDCH(node->ltag, ch);
				node->part_word = 0;
			} else
				node->backslash = 1;
		} else if (IS_SPACE(ch) || ch == ':') {
			/* 标签名分析结束，下一步需要找到冒号 */
			json->status = ACL_JSON_S_COLON;
			node->part_word = 0;
			break;
		}

		/* 是否兼容后半个汉字为转义符 '\' 的情况 */
		else if ((json->flag & ACL_JSON_FLAG_PART_WORD)) {
			ADDCH(node->ltag, ch);

			/* 处理半个汉字的情形 */
			if (node->part_word)
				node->part_word = 0;
			else if (ch < 0)
				node->part_word = 1;
		} else {
			ADDCH(node->ltag, ch);
		}
		data++;
	}

	/* 如果标签名非空，则需要保证以 0 结尾 */
	if (LEN(node->ltag) > 0)
		ACL_VSTRING_TERMINATE(node->ltag);

	return data;
}

/* 一直查到冒号为止，然后切换至分析标签值过程 */

static const char *json_colon(ACL_JSON *json, const char *data)
{
	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	if (*data != ':') {
		data++;
		return data;
	}

	data++;

	/* 下一步分析标签名所对应的标签值，有可能为字符串，
	 * 也有可能为子节点对象
	 */
	json->status = ACL_JSON_S_VALUE;

	return data;
}

static const char *json_array(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *array;

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	/* 创建数组对象 */
	array = acl_json_node_alloc(json);
	array->left_ch = '[';
	array->right_ch = ']';
	array->type = ACL_JSON_T_ARRAY;
	array->depth = json->curr_node->depth + 1;
	if (array->depth > json->depth)
		json->depth = array->depth;

	acl_json_node_add_child(json->curr_node, array);

	if (LEN(json->curr_node->ltag) > 0)
		json->curr_node->tag_node = array;

	json->curr_node = array;
	json->status = ACL_JSON_S_ELEMENT;

	/* 如果该数组为空，则直接查询其兄弟节点 */
	if (*data == ']') {
		json->status = ACL_JSON_S_NEXT;
		data++;
	}

	return data;
}

static const char *json_element(ACL_JSON *json, const char *data)
{
	/* 创建数组成员对象 */
	ACL_JSON_NODE *element;

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	if (*data == '{') {
		data++;
		json->status = ACL_JSON_S_OBJ;
		return data;
	} else if (*data == '[') {
		data++;
		json->status = ACL_JSON_S_ARRAY;
		return data;
	}

	element = acl_json_node_alloc(json);
	element->type = ACL_JSON_T_ELEMENT;
	element->depth = json->curr_node->depth + 1;
	if (element->depth > json->depth)
		json->depth = element->depth;

	acl_json_node_add_child(json->curr_node, element);

	/* 将该数组成员对象置为当前 JSON 分析节点 */
	json->curr_node = element;
	json->status = ACL_JSON_S_VALUE;

	return data;
}

/* 分析标签值，该值有可能是纯文本(即该节点为叶节点)，也有可能是一个子节点 */

static const char *json_value(ACL_JSON *json, const char *data)
{
	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	/* 为 '{' 或 '[' 时说明遇到了当前节点的子节点 */
	if (*data == '{') {
		data++;
		json->status = ACL_JSON_S_OBJ;
	} else if (*data == '[') {
		data++;
		json->status = ACL_JSON_S_ARRAY;
	}

	/* 兼容一下有些数据格式为 "xxx: ," 的方式 */
	else if (*data == ',' || *data == ';') {
		data++;
		/* 切换至查询该节点的兄弟节点的过程 */
		json->status = ACL_JSON_S_NEXT;
	}

	/* 说明标签名后面的标签值为字符串或数字 */
	/* 如果标签值前有引号，记录下该引号 */
	else if (IS_QUOTE(*data)) { /* && json->curr_node->quote == 0) { */
		json->curr_node->quote = *data++;
		json->status = ACL_JSON_S_STRING;
	} else
		json->status = ACL_JSON_S_STRING;

	json->curr_node->type = ACL_JSON_T_LEAF;
	return data;
}

static const char *json_string(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *node = json->curr_node;
	int   ch;

	/* Bugfix: 这防止字符串开始部分为空格时被忽略需注掉下面过滤逻辑 */
#if 0
	/* 当文本长度为 0 时，可以认为还未遇到有效的字符 */

	if (LEN(node->text) == 0) {
		/* 先过滤开头没用的空格 */
		//SKIP_SPACE(data);
		if (*data == 0)
			return data;
	}
#endif

	/* 说明本节点是叶节点 */

	while ((ch = *data) != 0) {
		/* 如果开始有引号，则需要以该引号作为结尾符 */
		if (node->quote) {
			if (node->backslash) {
				if (ch == 'b')
					ADDCH(node->text, '\b');
				else if (ch == 'f')
					ADDCH(node->text, '\f');
				else if (ch == 'n')
					ADDCH(node->text, '\n');
				else if (ch == 'r')
					ADDCH(node->text, '\r');
				else if (ch == 't')
					ADDCH(node->text, '\t');
				else
					ADDCH(node->text, ch);
				node->backslash = 0;
			}

			/* 当为双字节汉字时，第一个字节为的高位为 1，
			 * 第二个字节有可能为 92，正好与转义字符相同
			 */
			else if (ch == '\\') {
				/* 处理半个汉字的情况，如果前一个字节是前
				 * 半个汉字，则当前的转义符当作后半个汉字
				 */
				if (node->part_word) {
					ADDCH(node->text, ch);
					node->part_word = 0;
				} else
					node->backslash = 1;
			} else if (ch == node->quote) {
				/* 对节点的值，必须保留该 quote 值，以便于区分
				 * 不同的值类型：bool, null, number, string
				 * node->quote = 0;
				 */

				/* 切换至查询该节点的兄弟节点的过程 */
				json->status = ACL_JSON_S_STREND;
				node->part_word = 0;
				data++;
				break;
			}

			/* 是否兼容后半个汉字为转义符 '\' 的情况 */
			else if ((json->flag & ACL_JSON_FLAG_PART_WORD)) {
				ADDCH(node->text, ch);

				/* 若前一个字节为前半个汉字，则当前字节
				 * 为后半个汉字，正好为一个完整的汉字
				 */
				if (node->part_word)
					node->part_word = 0;

				/* 前一个字节非前半个汉字且当前字节高位
				 * 为 1，则表明当前字节为前半个汉字
				 */
				else if (ch < 0)
					node->part_word = 1;
			} else {
				ADDCH(node->text, ch);
			}
		} else if (node->backslash) {
			ADDCH(node->text, ch);
			node->backslash = 0;
		} else if (ch == '\\') {
			if (node->part_word) {
				ADDCH(node->text, ch);
				node->part_word = 0;
			} else
				node->backslash = 1;
		} else if (IS_SPACE(ch) || ch == ',' || ch == ';'
			|| ch == '}' || ch == ']')
		{
			/* 切换至查询该节点的兄弟节点的过程 */
			json->status = ACL_JSON_S_STREND;
			break;
		}

		/* 是否兼容后半个汉字为转义符 '\' 的情况 */
		else if ((json->flag & ACL_JSON_FLAG_PART_WORD)) {
			ADDCH(node->text, ch);

			/* 处理半个汉字的情形 */
			if (node->part_word)
				node->part_word = 0;
			else if (ch < 0)
				node->part_word = 1;
		} else {
			ADDCH(node->text, ch);
		}
		data++;
	}

	if (LEN(node->text) > 0)
		ACL_VSTRING_TERMINATE(node->text);

	return data;
}

static const char *json_strend(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *node = json->curr_node;
	ACL_JSON_NODE *parent;

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

#define	EQ(x, y) !strcasecmp((x), ((y)))
#define	IS_NUMBER(x) (acl_alldig((x)) \
		|| ((*(x) == '-' || *(x) == '+') \
			&& *((x) + 1) != 0 && acl_alldig((x) + 1)))

	if (node->parent && node->parent->type == ACL_JSON_T_ARRAY) {
		if (node->quote == 0) {
			const char* txt = STR(node->text);

			if (EQ(txt, "null"))
				node->type = ACL_JSON_T_A_NULL
					| ACL_JSON_T_LEAF;
			else if (EQ(txt, "true") || EQ(txt, "false"))
				node->type = ACL_JSON_T_A_BOOL
					| ACL_JSON_T_LEAF;
			else if (IS_NUMBER(txt))
				node->type = ACL_JSON_T_A_NUMBER
					| ACL_JSON_T_LEAF;
			else if (acl_is_double(txt))
				node->type = ACL_JSON_T_A_DOUBLE
					| ACL_JSON_T_LEAF;
			else
				node->type = ACL_JSON_T_A_STRING
					| ACL_JSON_T_LEAF;
		} else
			node->type = ACL_JSON_T_A_STRING | ACL_JSON_T_LEAF;
	} else if (node->quote == 0) {
		const char* txt = STR(node->text);

		if (EQ(txt, "null"))
			node->type = ACL_JSON_T_NULL | ACL_JSON_T_LEAF;
		else if (EQ(txt, "true") || EQ(txt, "false"))
			node->type = ACL_JSON_T_BOOL | ACL_JSON_T_LEAF;
		else if (IS_NUMBER(txt))
			node->type = ACL_JSON_T_NUMBER | ACL_JSON_T_LEAF;
		else if (acl_is_double(txt))
			node->type = ACL_JSON_T_DOUBLE | ACL_JSON_T_LEAF;
		else
			node->type = ACL_JSON_T_STRING | ACL_JSON_T_LEAF;
	} else
		node->type = ACL_JSON_T_STRING | ACL_JSON_T_LEAF;


	if (*data == ',' || *data == ';') {
		json->status = ACL_JSON_S_NEXT;
		return data;
	}

	parent = acl_json_node_parent(json->curr_node);
	if (*data != parent->right_ch) {  /* xxx */
		data++;
		return data;
	}

	data++;
	if (parent == json->root) {
		json->finish = 1;
		return data;
	}

	json->curr_node = parent;
	json->status = ACL_JSON_S_NEXT;
	return data;
}

/* 尝试分析本节点的下一个兄弟节点，必须能找到分隔符 ',' 或 ';' */

static const char *json_brother(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *parent;

	if (json->curr_node == json->root) {
		json->finish = 1;
		return data;
	}

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	/* 如果到达根节点的结束符，则 json 解析过程完毕 */
	parent = acl_json_node_parent(json->curr_node);
	acl_assert(parent);

	if (*data == ',' || *data == ';') {
		data++;

		if (parent->left_ch == '{')
			json->status = ACL_JSON_S_MEMBER;
		else if (parent->left_ch == '[')
			json->status = ACL_JSON_S_ELEMENT;
		else
			json->status = ACL_JSON_S_NEXT;

		json->curr_node = parent;
		return data;
	}

	if (*data == parent->right_ch) {
		data++;
		if (parent == json->root) {
			json->finish = 1;
			return data;
		}

		json->curr_node = parent;
		/* 查询父节点的下一个兄弟节点 */
		json->status = ACL_JSON_S_NEXT;
		return data;
	}

	if (parent->left_ch == '{')
		json->status = ACL_JSON_S_MEMBER;
	else if (parent->left_ch == '[')
		json->status = ACL_JSON_S_ELEMENT;
	else
		json->status = ACL_JSON_S_NEXT;

	json->curr_node = parent;
	return data;
}

/* 状态机数据结构类型 */

struct JSON_STATUS_MACHINE {
	/* 状态码 */
	int   status;

	/* 状态机处理函数 */
	const char *(*callback) (ACL_JSON*, const char*);
};

static struct JSON_STATUS_MACHINE status_tab[] = {
	{ ACL_JSON_S_ROOT,	json_root },    /* json root node */
	{ ACL_JSON_S_OBJ,	json_obj },     /* json obj node */
	{ ACL_JSON_S_MEMBER,	json_member },
	{ ACL_JSON_S_ARRAY,	json_array },   /* json array node */
	{ ACL_JSON_S_ELEMENT,	json_element },
	{ ACL_JSON_S_PAIR,	json_pair },    /* json pair node */
	{ ACL_JSON_S_NEXT,	json_brother }, /* json brother node */
	{ ACL_JSON_S_TAG,	json_tag },     /* json tag name */
	{ ACL_JSON_S_VALUE,	json_value },   /* json node's value */
	{ ACL_JSON_S_COLON,	json_colon },	/* json tag's ':' */
	{ ACL_JSON_S_STRING,	json_string },
	{ ACL_JSON_S_STREND,	json_strend },
};

const char* acl_json_update(ACL_JSON *json, const char *data)
{
	const char *ptr = data;

	if (data == NULL)
		return "";

	/* 检查是否已经解析完毕 */
	if (json->finish)
		return ptr;

	/* json 解析器状态机循环处理过程 */

	while (*ptr && !json->finish)
		ptr = status_tab[json->status].callback(json, ptr);

	return ptr;
}

int acl_json_finish(ACL_JSON *json)
{
	return json->finish;
}
