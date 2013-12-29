#include "StdAfx.h"
#include <stdio.h>
#ifndef ACL_PREPARE_COMPILE
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


/* 分析结点对象值，必须找到 '{' 或 '[' */

static const char *json_obj(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *obj, *member;

	SKIP_SPACE(data);

	/* 必须找到子结点的开始字符 */
	SKIP_WHILE(*data != '{', data);

	if (*data == 0)
		return NULL;

	/* 创建对象 '{}' 子结点 */

	obj = acl_json_node_alloc(json);
	acl_json_node_add_child(json->curr_node, obj);
	obj->depth = json->curr_node->depth + 1;

	/* 根据 json 结点对象前缀的不同，记录不同的对象后缀 */
	obj->left_ch = *data;
	obj->right_ch = '}';
	obj->type = ACL_JSON_T_OBJ;

	/* 去掉前导符 '{' 或 '[' */
	data++;

	json->curr_node->tag_node = obj;

	/* 创建上面所建对象结点的成员对象 */
	member = acl_json_node_alloc(json);

	acl_json_node_add_child(obj, member);
	member->type = ACL_JSON_T_MEMBER;
	member->depth = obj->depth + 1;
	if (member->depth > json->depth)
		json->depth = member->depth;

	/* 将该成员对象置为当前 JSON 分析结点 */
	json->curr_node = member;
	json->status = ACL_JSON_S_PAR;

	return data;
}

static const char *json_array(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *array, *element;

	SKIP_SPACE(data);
	SKIP_WHILE(*data != '[', data);
	if (*data == 0)
		return NULL;

	/* 创建数组对象 */
	array = acl_json_node_alloc(json);
	acl_json_node_add_child(json->curr_node, array);
	array->left_ch = *data;
	array->right_ch = ']';
	array->type = ACL_JSON_T_ARRAY;
	data++;

	json->curr_node->tag_node = array;

	/* 创建数组成员对象 */
	element = acl_json_node_alloc(json);
	acl_json_node_add_child(array, element);
	element->type = ACL_JSON_T_ELEMENT;
	element->depth = array->depth + 1;
	if (element->depth > json->depth)
		json->depth = element->depth;

	/* 将该数组成员对象置为当前 JSON 分析结点 */
	json->curr_node = element;
	json->status = ACL_JSON_S_VAL;

	return data;
}

/* 解析结点的标签名称，结点允许没有标签名；叶结点没有 { } [ ] 分隔符 */

static const char *json_pair(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *parent = acl_json_node_parent(json->curr_node);

	acl_assert(parent);
	SKIP_SPACE(data);
	if (*data == 0)
		return NULL;

	/* 如果当前字符为父结点的右分隔符，则表示父结点结束 */
	if (*data == parent->right_ch) {
		data++;  /* 去掉父结点的右分隔符 */

		if (parent == json->root) {
			/* 如果根结点分析结束则整个 json 分析完毕 */
			json->finish = 1;
			return NULL;
		}
		/* 弹出父结点 */
		json->curr_node = parent;
		/* 查询父结点的下一个兄弟结点 */
		json->status = ACL_JSON_S_NXT;
		return data;
	}

	/* 为 '{' 或 '[' 时说明遇到了当前结点的子结点 */
	if (*data == '{') {
		json->status = ACL_JSON_S_OBJ;
		return data;
	}
	else if (*data == '[') {
		json->status = ACL_JSON_S_ARR;
		return data;
	}

	/* 如果标签名前有引号，记录下该引号 */
	if (IS_QUOTE(*data) && json->curr_node->quote == 0)
		json->curr_node->quote = *data++;

	json->curr_node->type = ACL_JSON_T_PAIR;
	json->status = ACL_JSON_S_TAG;

	return data;
}

/* 尝试分析本结点的下一个兄弟结点，必须能找到分隔符 ',' 或 ';' */

static const char *json_next(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *parent, *brother;

	if (json->curr_node == json->root) {
		json->finish = 1;
		return NULL;
	}

	SKIP_SPACE(data);
	if (*data == 0)
		return NULL;

	/* 如果到达根结点的结束符，则 json 解析过程完毕 */
	parent = acl_json_node_parent(json->curr_node);
	if (*data == parent->right_ch) {
		data++;
		if (parent == json->root) {
			json->finish = 1;
			return NULL;
		}

		/* 弹出爷爷结点 */
		parent = acl_json_node_parent(parent);
		json->curr_node = parent;
		/* 查询父结点的下一个兄弟结点 */
		json->status = ACL_JSON_S_NXT;
		return data;
	}

	/* 必须得先找到分隔符: ',' 或 ';' 才能启动下一个兄弟结点的解析过程 */

	SKIP_WHILE(*data != ',' && *data != ';', data);
	if (*data == 0)
		return NULL;

	data++;

	/* 创建新结点，并将其作为当前结点的兄弟结点 */

	brother = acl_json_node_alloc(json);
	acl_json_node_add_child(parent, brother);
	brother->depth = parent->depth;

	/* 如果父结点为数组对象，则将解析器转向值解析过程 */
	if (parent->left_ch == '[')
		json->status = ACL_JSON_S_VAL;

	/* 如果父结点为对象，则将解析器转向 NAME:VALUE 对解析过程 */
	else if (parent->left_ch == '{')
		json->status = ACL_JSON_S_PAR;

	/* xxx: 否则，则有可能数据是问题的 */
	else
		json->status = ACL_JSON_S_VAL;

	json->curr_node = brother;
	return data;
}

/* 解析结点的标签名称，结点允许没有标签名；叶结点没有 { } [ ] 分隔符 */

static const char *json_tag(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *node = json->curr_node;
	int   ch;

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

				/* 数组对象的子结点允许为单独的字符串或对象 */
				if (parent->left_ch == '[')
					json->status = ACL_JSON_S_NXT;

				/* 标签值分析结束，下一步需要找到冒号 */
				else
					json->status = ACL_JSON_S_COL;
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
			json->status = ACL_JSON_S_COL;
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
		return NULL;

	if (*data != ':') {
		data++;
		return data;
	}

	data++;

	/* 下一步分析标签名所对应的标签值，有可能为字符串，
	 * 也有可能为子结点对象
	 */
	json->status = ACL_JSON_S_VAL;

	return data;
}

/* 分析标签值，该值有可能是纯文本(即该结点为叶结点)，也有可能是一个子结点 */

static const char *json_val(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *node = json->curr_node;
	int   ch;

	/* 当文本长度为 0 时，可以认为还未遇到有效的字符 */

	if (LEN(node->text) == 0 && node->quote == 0) {
		/* 先过滤开头没用的空格 */
		SKIP_SPACE(data);
		SKIP_WHILE(*data == ':', data);
		if (*data == 0)
			return NULL;

		/* 为 '{' 或 '[' 时说明遇到了当前结点的子结点 */
		if (*data == '{') {
			json->status = ACL_JSON_S_OBJ;
			return data;
		} else if (*data == '[') {
			json->status = ACL_JSON_S_ARR;
			return data;
		}

		/* 兼容一下有些数据格式为 "xxx: ," 的方式 */
		if (*data == ',' || *data == ';') {
			/* 切换至查询该结点的兄弟结点的过程 */
			json->status = ACL_JSON_S_NXT;
			return data;
		}

		/* 说明标签名后面的标签值为字符串或数字 */

		/* 如果标签值前有引号，记录下该引号 */
		if (IS_QUOTE(*data) && node->quote == 0)
			node->quote = *data++;
	}

	/* 说明本结点是叶结点 */

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
				node->quote = 0;

				/* 切换至查询该结点的兄弟结点的过程 */
				json->status = ACL_JSON_S_NXT;
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
				ACL_VSTRING_TERMINATE(node->text);
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
			/* 切换至查询该结点的兄弟结点的过程 */
			json->status = ACL_JSON_S_NXT;
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

/* 状态机数据结构类型 */

struct JSON_STATUS_MACHINE {
	/* 状态码 */
	int   status;

	/* 状态机处理函数 */
	const char *(*callback) (ACL_JSON*, const char*);
};

static struct JSON_STATUS_MACHINE status_tab[] = {
	{ ACL_JSON_S_OBJ,  json_obj },          /* json obj node */
	{ ACL_JSON_S_ARR,  json_array },        /* json array node */
	{ ACL_JSON_S_PAR,  json_pair },         /* json pair node */
	{ ACL_JSON_S_NXT,  json_next },         /* json brother node */
	{ ACL_JSON_S_TAG,  json_tag },          /* json tag name */
	{ ACL_JSON_S_VAL,  json_val },          /* json node's value */
	{ ACL_JSON_S_COL,  json_colon },        /* json tag's ':' */
};

void acl_json_update(ACL_JSON *json, const char *data)
{
	const char *ptr = data;

	/* 检查是否已经解析完毕 */
	if (json->finish)
		return;

	/* json 解析器状态机循环处理过程 */

	while (ptr && *ptr) {
		ptr = status_tab[json->status].callback(json, ptr);
	}
}
