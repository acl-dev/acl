#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include <stdio.h>
#include "stdlib/acl_mystring.h"
#include "code/acl_xmlcode.h"
#include "xml/acl_xml.h"

#endif

#define	ADDCH	ACL_VSTRING_ADDCH
#define	LEN	ACL_VSTRING_LEN
#define	STR	acl_vstring_str
#define END	acl_vstring_end
#define	STRCPY	acl_vstring_strcpy

#define IS_DOCTYPE(ptr) ((*(ptr) == 'd' || *(ptr) == 'D')  \
	&& (*(ptr + 1) == 'o' || *(ptr + 1) == 'O')  \
	&& (*(ptr + 2) == 'c' || *(ptr + 2) == 'C')  \
	&& (*(ptr + 3) == 't' || *(ptr + 3) == 'T')  \
	&& (*(ptr + 4) == 'y' || *(ptr + 4) == 'Y')  \
	&& (*(ptr + 5) == 'p' || *(ptr + 5) == 'P')  \
	&& (*(ptr + 5) == 'E' || *(ptr + 6) == 'E'))

#define IS_ID(ptr) ((*(ptr) == 'i' || *(ptr) == 'I')  \
	&& (*(ptr + 1) == 'd' || *(ptr + 1) == 'D'))

#define IS_QUOTE(x) ((x) == '\"' || (x) == '\'')
#if 1
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#else
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t')
#endif
#define SKIP_WHILE(cond, ptr) { while(*(ptr) && (cond)) (ptr)++; }
#define SKIP_SPACE(ptr) { while(IS_SPACE(*(ptr))) (ptr)++; }

/* 状态机数据结构类型 */

struct XML_STATUS_MACHINE {
	/**< 状态码 */
	int   status;

	/**< 状态机处理函数 */
	const char *(*callback) (ACL_XML*, const char*);
};

static void xml_parse_check_self_closed(ACL_XML *xml)
{
	if ((xml->curr_node->flag & ACL_XML_F_LEAF) == 0) {
		if (acl_xml_tag_leaf(STR(xml->curr_node->ltag))) {
			xml->curr_node->flag |= ACL_XML_F_LEAF;
		}
	}

	if ((xml->curr_node->flag & ACL_XML_F_SELF_CL) == 0) {
		if (xml->curr_node->last_ch == '/'
		    || acl_xml_tag_selfclosed(STR(xml->curr_node->ltag)))
		{
			xml->curr_node->flag |= ACL_XML_F_SELF_CL;
		}
	}
}

static const char *xml_parse_next_left_lt(ACL_XML *xml, const char *data)
{
	SKIP_SPACE(data);
	SKIP_WHILE(*data != '<', data);
	if (*data == 0)
		return data;
	data++;
	xml->curr_node->status = ACL_XML_S_LLT;
	return data;
}

static const char *xml_parse_left_lt(ACL_XML *xml, const char *data)
{
	xml->curr_node->status = ACL_XML_S_LCH;
	return data;
}

static const char *xml_parse_left_ch(ACL_XML *xml, const char *data)
{
	int  ch = *data;

	if (ch == '!') {
		xml->curr_node->meta[0] = ch;
		xml->curr_node->status = ACL_XML_S_LEM;
		data++;
	} else if (ch == '?') {
		xml->curr_node->meta[0] = ch;
		xml->curr_node->flag |= ACL_XML_F_META_QM;
		xml->curr_node->status = ACL_XML_S_MTAG;
		data++;
	} else
		xml->curr_node->status = ACL_XML_S_LTAG;

	return data;
}

static const char *xml_parse_left_em(ACL_XML *xml, const char *data)
{
	if (*data == '-') {
		if (xml->curr_node->meta[1] != '-')
			xml->curr_node->meta[1] = '-';
		else if (xml->curr_node->meta[2] != '-') {
			xml->curr_node->meta[0] = 0;
			xml->curr_node->meta[1] = 0;
			xml->curr_node->meta[2] = 0;
			xml->curr_node->flag |= ACL_XML_F_META_CM;
			xml->curr_node->status = ACL_XML_S_MCMT;
		}

		data++;
	} else {
		if (xml->curr_node->meta[1] == '-') {
			ADDCH(xml->curr_node->ltag, '-');
			xml->curr_node->meta[1] = 0;
		}

		xml->curr_node->flag |= ACL_XML_F_META_EM;
		xml->curr_node->status = ACL_XML_S_MTAG;
	}

	ACL_VSTRING_TERMINATE(xml->curr_node->ltag);

	return data;
}

static const char *xml_parse_cdata(ACL_XML *xml, const char *data)
{
	ACL_XML_NODE *curr_node = xml->curr_node;
	int   ch;

	while ((ch = *data) != 0) {
		data++;

		if (ch == '>') {
			if (curr_node->meta[0] == ']'
				&& curr_node->meta[1] == ']')
			{
				curr_node->status = ACL_XML_S_MEND;
				ACL_VSTRING_TERMINATE(curr_node->text);
				return data;
			}
			if (curr_node->meta[0])
				ADDCH(curr_node->text, curr_node->meta[0]);
			curr_node->meta[0] = 0;
			if (curr_node->meta[1])
				ADDCH(curr_node->text, curr_node->meta[1]);
			curr_node->meta[1] = 0;
		} else if (ch == ']') {
			if (curr_node->meta[0] == ']') {
				if (curr_node->meta[1] == ']')
					ADDCH(curr_node->text, ']');
				else
					curr_node->meta[1] = ']';
			} else if (curr_node->meta[1] == ']') {
				curr_node->meta[0] = ']';
				curr_node->meta[1] = 0;
				ADDCH(curr_node->text, ']');
			} else
				curr_node->meta[0] = ']';
		} else if (curr_node->meta[0] == ']') {
			ADDCH(curr_node->text, ']');
			curr_node->meta[0] = 0;
			if (curr_node->meta[1] == ']')
				ADDCH(curr_node->text, ']');
			curr_node->meta[1] = 0;
		} else {
			ADDCH(curr_node->text, ch);
		}
	}

	ACL_VSTRING_TERMINATE(curr_node->text);
	return data;
}

#define	IS_CDATA(x) (*(x) == '[' \
	&& (*(x + 1) == 'C' || *(x + 1) == 'c') \
	&& (*(x + 2) == 'D' || *(x + 2) == 'd') \
	&& (*(x + 3) == 'A' || *(x + 3) == 'a') \
	&& (*(x + 4) == 'T' || *(x + 4) == 't') \
	&& (*(x + 5) == 'A' || *(x + 5) == 'a') \
	&& *(x + 6) == '[')

#define CDATA_S sizeof("[CDATA[") - 1

static void cdata_prepare(ACL_XML_NODE *curr_node)
{
	char *ptr;

	ACL_VSTRING_TERMINATE(curr_node->ltag);
	ptr = STR(curr_node->ltag) + sizeof("[CDATA[") - 1;

	if (*ptr)
		acl_vstring_strcpy(curr_node->text, ptr);
	ACL_VSTRING_AT_OFFSET(curr_node->ltag, sizeof("[CDATA[") - 1);
	ACL_VSTRING_TERMINATE(curr_node->ltag);
}

static const char *xml_parse_meta_tag(ACL_XML *xml, const char *data)
{
	int   ch;

	while ((ch = *data) != 0) {
#if 0
		if (IS_SPACE(ch) || ch == '>' || ch == ']') {
			if (IS_CDATA(STR(xml->curr_node->ltag))) {
				cdata_prepare(xml->curr_node);
				ADDCH(xml->curr_node->text, ch);
				xml->curr_node->status = ACL_XML_S_CDATA;
				xml->curr_node->flag |= ACL_XML_F_CDATA;
			} else
				xml->curr_node->status = ACL_XML_S_MTXT;
			break;
		}
#else
		if (LEN(xml->curr_node->ltag) >= CDATA_S
			&& IS_CDATA(STR(xml->curr_node->ltag)))
		{
			cdata_prepare(xml->curr_node);
			xml->curr_node->status = ACL_XML_S_CDATA;
			xml->curr_node->flag |= ACL_XML_F_CDATA;
			break;
		} else if (IS_SPACE(ch) || ch == '>') {
			xml->curr_node->status = ACL_XML_S_MTXT;
			data++;
			break;
		}
#endif
		data++;

		ADDCH(xml->curr_node->ltag, ch);
	}

	ACL_VSTRING_TERMINATE(xml->curr_node->ltag);
	return data;
}

static const char *xml_meta_attr_name(ACL_XML_ATTR *attr, const char *data)
{
	int   ch;

	while ((ch = *data) != 0) {
		if (ch == '=') {
			data++;
			ACL_VSTRING_TERMINATE(attr->name);
			break;
		}
		if (!IS_SPACE(ch))
			ADDCH(attr->name, ch);
		data++;
	}

	return data;
}

static const char *xml_meta_attr_value(ACL_XML_ATTR *attr, const char *data)
{
	int   ch;

	SKIP_SPACE(data);
	if (IS_QUOTE(*data))
		attr->quote = *data++;

	if (*data == 0)
		return data;

	while ((ch = *data) != 0) {
		if (attr->quote) {
			if (ch == attr->quote) {
				data++;
				break;
			}
			ADDCH(attr->value, ch);
		} else if (IS_SPACE(ch)) {
			data++;
			break;
		} else
			ADDCH(attr->value, ch);
		data++;
	}

	ACL_VSTRING_TERMINATE(attr->value);
	return data;
}

static void xml_meta_attr(ACL_XML_NODE *node)
{
	ACL_XML_ATTR *attr;
	const char *ptr;
	int   ch;

	ptr = STR(node->text);
	SKIP_SPACE(ptr);	/* 略过 ' ', '\t' */

	if (*ptr == 0)
		return;

	while ((ch = *ptr) != 0) {
		attr = acl_xml_attr_alloc(node);
		ptr = xml_meta_attr_name(attr, ptr);
		if (*ptr == 0)
			break;
		ptr = xml_meta_attr_value(attr, ptr);
		if (*ptr == 0)
			break;
	}
}

static const char *xml_parse_meta_text(ACL_XML *xml, const char *data)
{
	int   ch;

	if (LEN(xml->curr_node->text) == 0)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	while ((ch = *data) != 0) {
		if (xml->curr_node->quote) {
			if (ch == xml->curr_node->quote)
				xml->curr_node->quote = 0;
			ADDCH(xml->curr_node->text, ch);
		} else if (IS_QUOTE(ch)) {
			if (xml->curr_node->quote == 0)
				xml->curr_node->quote = ch;
			ADDCH(xml->curr_node->text, ch);
		} else if (ch == '<') {
			xml->curr_node->nlt++;
			ADDCH(xml->curr_node->text, ch);
		} else if (ch != '>') {
			ADDCH(xml->curr_node->text, ch);
		} else if (xml->curr_node->nlt == 0) {
			char *last;
			int off;

			data++;
			xml->curr_node->status = ACL_XML_S_MEND;
			if ((xml->curr_node->flag & ACL_XML_F_META_QM) == 0)
				break;

			last = acl_vstring_end(xml->curr_node->text) - 1;
			if (last < STR(xml->curr_node->text) || *last != '?')
				break;

			off = (int) ACL_VSTRING_LEN(xml->curr_node->text) - 1;
			if (off == 0)
				break;

			ACL_VSTRING_AT_OFFSET(xml->curr_node->text, off);
			ACL_VSTRING_TERMINATE(xml->curr_node->text);

			xml_meta_attr(xml->curr_node);
			break;
		} else {
			xml->curr_node->nlt--;
			ADDCH(xml->curr_node->text, ch);
		}

		data++;
	}

	ACL_VSTRING_TERMINATE(xml->curr_node->text);
	return data;
}

static const char *xml_parse_meta_comment(ACL_XML *xml, const char *data)
{
	int   ch;

	if (LEN(xml->curr_node->text) == 0)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	while ((ch = *data) != 0) {
		if (xml->curr_node->quote) {
			if (ch == xml->curr_node->quote)
				xml->curr_node->quote = 0;
			else
				ADDCH(xml->curr_node->text, ch);
		} else if (IS_QUOTE(ch)) {
			if (xml->curr_node->quote == 0)
				xml->curr_node->quote = ch;
			else
				ADDCH(xml->curr_node->text, ch);
		} else if (ch == '<') {
			xml->curr_node->nlt++;
			ADDCH(xml->curr_node->text, ch);
		} else if (ch == '>') {
			if (xml->curr_node->nlt == 0
				&& xml->curr_node->meta[0] == '-'
				&& xml->curr_node->meta[1] == '-')
			{
				data++;
				xml->curr_node->status = ACL_XML_S_MEND;
				break;
			}

			xml->curr_node->nlt--;
			ADDCH(xml->curr_node->text, ch);
		} else if (xml->curr_node->nlt > 0) {
			ADDCH(xml->curr_node->text, ch);
		} else if (ch == '-') {
			if (xml->curr_node->meta[0] != '-')
				xml->curr_node->meta[0] = '-';
			else if (xml->curr_node->meta[1] != '-')
				xml->curr_node->meta[1] = '-';
		} else {
			if (xml->curr_node->meta[0] == '-') {
				ADDCH(xml->curr_node->text, '-');
				xml->curr_node->meta[0] = 0;
			}
			if (xml->curr_node->meta[1] == '-') {
				ADDCH(xml->curr_node->text, '-');
				xml->curr_node->meta[1] = 0;
			}
			ADDCH(xml->curr_node->text, ch);
		}

		data++;
	}

	ACL_VSTRING_TERMINATE(xml->curr_node->text);
	return data;
}

static const char *xml_parse_meta_end(ACL_XML *xml, const char *data)
{
	/* meta 标签是自关闭类型，直接跳至右边 '>' 处理位置 */
	xml->curr_node->status = ACL_XML_S_RGT;
	return data;
}

static const char *xml_parse_left_tag(ACL_XML *xml, const char *data)
{
	int   ch;

	if (LEN(xml->curr_node->ltag) == 0)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	while ((ch = *data) != 0) {
		data++;

		if (ch == '>') {
			xml_parse_check_self_closed(xml);

			if ((xml->curr_node->flag & ACL_XML_F_SELF_CL)
				&& xml->curr_node->last_ch == '/')
			{
				acl_vstring_truncate(xml->curr_node->ltag,
					LEN(xml->curr_node->ltag) - 1);
				xml->curr_node->status = ACL_XML_S_RGT;
			} else
				xml->curr_node->status = ACL_XML_S_LGT;
			break;
		} else if (IS_SPACE(ch)) {
			xml->curr_node->status = ACL_XML_S_ATTR;
			xml->curr_node->last_ch = ch;
			break;
		} else {
			ADDCH(xml->curr_node->ltag, ch);
			xml->curr_node->last_ch = ch;
		}
	}

	ACL_VSTRING_TERMINATE(xml->curr_node->ltag);
	return data;
}

static const char *xml_parse_attr(ACL_XML *xml, const char *data)
{
	int   ch;
	ACL_XML_ATTR *attr = xml->curr_node->curr_attr;

	if (attr == NULL || LEN(attr->name) == 0) {
		SKIP_SPACE(data);	/* 略过 ' ', '\t' */
		SKIP_WHILE(*data == '=', data);
	}

	if (*data == 0)
		return data;

	if (*data == '>') {
		xml_parse_check_self_closed(xml);

		if ((xml->curr_node->flag & ACL_XML_F_SELF_CL)
			&& xml->curr_node->last_ch == '/')
		{
			xml->curr_node->status = ACL_XML_S_RGT;
		} else
			xml->curr_node->status = ACL_XML_S_LGT;

		xml->curr_node->curr_attr = NULL;
		data++;
		return data;
	}

	xml->curr_node->last_ch = *data;
	if (*data == '/') {
		data++;

		/* 此处返回后会触发本函数再次被调用，当下一个字节为 '>' 时，
		 * 上面通过调用 xml_parse_check_self_closed 检查是否为自封闭
		 * 标签: "/>"
		 */
		return data;
	}

	if (attr == NULL) {
		attr = acl_xml_attr_alloc(xml->curr_node);
		xml->curr_node->curr_attr = attr;
	}

	while ((ch = *data) != 0) {
		data++;
		xml->curr_node->last_ch = ch;
		if (ch == '=') {
			xml->curr_node->status = ACL_XML_S_AVAL;
			break;
		}
		if (!IS_SPACE(ch))
			ADDCH(attr->name, ch);
	}

	ACL_VSTRING_TERMINATE(attr->name);
	return data;
}

static const char *xml_parse_attr_val(ACL_XML *xml, const char *data)
{
	int   ch;
	ACL_XML_ATTR *attr = xml->curr_node->curr_attr;

	if (LEN(attr->value) == 0 && !attr->quote) {
		SKIP_SPACE(data);
		if (IS_QUOTE(*data))
			attr->quote = *data++;
	}

	if (*data == 0)
		return data;

	while ((ch = *data) != 0) {
		data++;

		if (attr->quote) {
			if (ch == attr->quote) {
				xml->curr_node->status = ACL_XML_S_ATTR;
				xml->curr_node->last_ch = ch;
				break;
			}
			ADDCH(attr->value, ch);
			xml->curr_node->last_ch = ch;
		} else if (ch == '>') {
			xml_parse_check_self_closed(xml);

			if ((xml->curr_node->flag & ACL_XML_F_SELF_CL)
				&& xml->curr_node->last_ch == '/')
			{
				acl_vstring_truncate(attr->value,
					LEN(attr->value) - 1);
				xml->curr_node->status = ACL_XML_S_RGT;
			} else
				xml->curr_node->status = ACL_XML_S_LGT;
			break;
		} else if (IS_SPACE(ch)) {
			xml->curr_node->status = ACL_XML_S_ATTR;
			xml->curr_node->last_ch = ch;
			break;
		} else {
			ADDCH(attr->value, ch);
			xml->curr_node->last_ch = ch;
		}
	}

	ACL_VSTRING_TERMINATE(attr->value);

	if (xml->curr_node->status != ACL_XML_S_AVAL) {
		if (LEN(attr->value) > 0 && xml->decode_buf != NULL) {
			ACL_VSTRING_RESET(xml->decode_buf);
			acl_xml_decode(STR(attr->value), xml->decode_buf);
			if (LEN(xml->decode_buf) > 0)
				STRCPY(attr->value, STR(xml->decode_buf));
		}

		/* 将该标签ID号映射至哈希表中，以便于快速查询 */
		if (IS_ID(STR(attr->name)) && LEN(attr->value) > 0) {
			const char *ptr = STR(attr->value);

			/* 防止重复ID被插入现象 */
			if (acl_htable_find(xml->id_table, ptr) == NULL) {
				acl_htable_enter(xml->id_table, ptr, attr);

				/* 当该属性被加入哈希表后才会赋于节点的 id */
				xml->curr_node->id = attr->value;
			}
		}

		/* 必须将该节点的当前属性对象置空，以便于继续解析时
		 * 可以创建新的属性对象
		 */
		xml->curr_node->curr_attr = NULL;
	}

	return data;
}

static const char *xml_parse_left_gt(ACL_XML *xml, const char *data)
{
	xml->curr_node->last_ch = 0;
	xml->curr_node->status = ACL_XML_S_TXT;
	return data;
}

static const char *xml_parse_text(ACL_XML *xml, const char *data)
{
	int   ch;

	if (LEN(xml->curr_node->text) == 0)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	while ((ch = *data) != 0) {
		data++;

		if (ch == '<') {
			xml->curr_node->status = ACL_XML_S_RLT;
			break;
		}

		ADDCH(xml->curr_node->text, ch);
	}

	ACL_VSTRING_TERMINATE(xml->curr_node->text);

	if (xml->curr_node->status != ACL_XML_S_RLT)
		return data;

	if (LEN(xml->curr_node->text) == 0 || xml->decode_buf == NULL)
		return data;

	ACL_VSTRING_RESET(xml->decode_buf);
	acl_xml_decode(STR(xml->curr_node->text), xml->decode_buf);
	if (LEN(xml->decode_buf) > 0)
		STRCPY(xml->curr_node->text, STR(xml->decode_buf));

	return data;
}

static const char *xml_parse_right_lt(ACL_XML *xml, const char *data)
{
	ACL_XML_NODE *node;

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	if (*data == '/') {
		data++;
		xml->curr_node->status = ACL_XML_S_RTAG;

		return data;
	} else if ((xml->curr_node->flag & ACL_XML_F_LEAF)) {
		ADDCH(xml->curr_node->text, '<');
		ADDCH(xml->curr_node->text, *data);
		ACL_VSTRING_TERMINATE(xml->curr_node->text);
		xml->curr_node->status = ACL_XML_S_TXT;
		data++;

		return data;
	}

	/* 说明遇到了当前节点的子节点 */

	/* 重新设置当前节点状态，以便于其被子节点弹出时可以找到 "</" */
	xml->curr_node->status = ACL_XML_S_TXT;

	/* 创建新的子节点，并将其加入至当前节点的子节点集合中 */

	node = acl_xml_node_alloc(xml);
	acl_xml_node_add_child(xml->curr_node, node);
	node->depth = xml->curr_node->depth + 1;
	if (node->depth > xml->depth)
		xml->depth = node->depth;
	xml->curr_node = node;
	xml->curr_node->status = ACL_XML_S_LLT;

	return data;
}

/* 因为该父节点其实为叶节点，所以需要更新附属于该伪父节点的
 * 子节点的深度值，都应与该伪父节点相同
 */ 
static void update_children_depth(ACL_XML_NODE *parent)
{
	ACL_ITER  iter;
	ACL_XML_NODE *child;

	acl_foreach(iter, parent) {
		child = (ACL_XML_NODE*) iter.data;
		child->depth = parent->depth;
		update_children_depth(child);
	}
}

/* 查找与右标签相同的父节点 */
static int search_match_node(ACL_XML *xml)
{
	ACL_XML_NODE *parent, *node;
	ACL_ARRAY *nodes = acl_array_create(10);
	ACL_ITER iter;

	parent = acl_xml_node_parent(xml->curr_node);
	if (parent != xml->root)
		acl_array_append(nodes, xml->curr_node);

	while (parent != xml->root) {
		if (acl_strcasecmp(STR(xml->curr_node->rtag),
			STR(parent->ltag)) == 0)
		{
			acl_vstring_strcpy(parent->rtag,
				STR(xml->curr_node->rtag));
			ACL_VSTRING_RESET(xml->curr_node->rtag);
			ACL_VSTRING_TERMINATE(xml->curr_node->rtag);
			parent->status = ACL_XML_S_RGT;
			xml->curr_node = parent;
			break;
		}

		acl_array_append(nodes, parent);

		parent = acl_xml_node_parent(parent);
	}

	if (parent == xml->root) {
		acl_array_free(nodes, NULL);
		return 0;
	}

	acl_foreach_reverse(iter, nodes) {
		node = (ACL_XML_NODE*) iter.data;
		acl_ring_detach(&node->node);
		node->flag |= ACL_XML_F_LEAF;
		node->depth = parent->depth + 1;
		update_children_depth(node);
		acl_xml_node_add_child(parent, node);
	}

	acl_array_free(nodes, NULL);

	return 1;
}

static const char *xml_parse_right_tag(ACL_XML *xml, const char *data)
{
	int   ch;
	ACL_XML_NODE *curr_node = xml->curr_node;

	/* after: "</" */

	if (LEN(curr_node->rtag) == 0)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	while ((ch = *data) != 0) {
		data++;

		if (ch == '>') {
			curr_node->status = ACL_XML_S_RGT;
			break;
		}

		if (!IS_SPACE(ch))
			ADDCH(curr_node->rtag, ch);
	}

	ACL_VSTRING_TERMINATE(curr_node->rtag);

	if (curr_node->status != ACL_XML_S_RGT)
		return data;

	if (acl_strcasecmp(STR(curr_node->ltag), STR(curr_node->rtag)) != 0) {
		int   ret;

		if ((xml->flag & ACL_XML_FLAG_IGNORE_SLASH))
			ret = search_match_node(xml);
		else
			ret = 0;

		if (ret == 0) {
			/* 如果节点标签名与开始标签名不匹配，
			 * 则需要继续寻找真正的结束标签
			 */ 
			acl_vstring_strcat(curr_node->text,
				STR(curr_node->rtag));
			ACL_VSTRING_RESET(curr_node->rtag);
			ACL_VSTRING_TERMINATE(curr_node->rtag);

			/* 重新设置当前节点状态，以便于其可以找到 "</" */
			curr_node->status = ACL_XML_S_TXT;
		}
	}

	return data;
}

static const char *xml_parse_right_gt(ACL_XML *xml, const char *data)
{
	/* 当前节点分析完毕，需要弹出当前节点的父节点继续分析 */
	ACL_XML_NODE *parent = acl_xml_node_parent(xml->curr_node);

	if (parent == xml->root) {
		if ((xml->curr_node->flag & ACL_XML_F_META) == 0)
			xml->root_cnt++;
		xml->curr_node = NULL;
	} else
		xml->curr_node = parent;

	return data;
}

static struct XML_STATUS_MACHINE status_tab[] = {
	{ ACL_XML_S_NXT,   xml_parse_next_left_lt        },
	{ ACL_XML_S_LLT,   xml_parse_left_lt             },
	{ ACL_XML_S_LGT,   xml_parse_left_gt             },
	{ ACL_XML_S_LCH,   xml_parse_left_ch             },
	{ ACL_XML_S_LEM,   xml_parse_left_em             },
	{ ACL_XML_S_LTAG,  xml_parse_left_tag            },
	{ ACL_XML_S_RLT,   xml_parse_right_lt            },
	{ ACL_XML_S_RGT,   xml_parse_right_gt            },
	{ ACL_XML_S_RTAG,  xml_parse_right_tag           },
	{ ACL_XML_S_ATTR,  xml_parse_attr                },
	{ ACL_XML_S_AVAL,  xml_parse_attr_val            },
	{ ACL_XML_S_TXT,   xml_parse_text                },
	{ ACL_XML_S_MTAG,  xml_parse_meta_tag            },
	{ ACL_XML_S_MTXT,  xml_parse_meta_text           },
	{ ACL_XML_S_MCMT,  xml_parse_meta_comment        },
	{ ACL_XML_S_MEND,  xml_parse_meta_end            },
	{ ACL_XML_S_CDATA, xml_parse_cdata               },
};

const char *acl_xml_update(ACL_XML *xml, const char *data)
{
	if (data == NULL)
		return "";
	else if (*data == 0)
		return data;

	if (!(xml->flag & ACL_XML_FLAG_MULTI_ROOT) && xml->root_cnt > 0)
		return data;

	/* XML 解析器状态机循环处理过程 */

	while (*data) {
		if (xml->curr_node == NULL) {
			if (!(xml->flag & ACL_XML_FLAG_MULTI_ROOT)
				&& xml->root_cnt > 0)
			{
				break;
			}

			SKIP_SPACE(data);
			if (*data == 0)
				break;
			xml->curr_node = acl_xml_node_alloc(xml);
			acl_xml_node_add_child(xml->root, xml->curr_node);
			xml->curr_node->depth = xml->root->depth + 1;
			if (xml->curr_node->depth > xml->depth)
				xml->depth = xml->curr_node->depth;
		}
		data = status_tab[xml->curr_node->status].callback(xml, data);
	}

	return data;
}
