#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include <stdio.h>
#include "stdlib/acl_mystring.h"
#include "code/acl_xmlcode.h"
#include "xml/acl_xml3.h"

#endif

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
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#define SKIP_WHILE(cond, ptr) { while(*(ptr) && (cond)) (ptr)++; }
#define SKIP_SPACE(ptr) { while(IS_SPACE(*(ptr))) (ptr)++; }

/* 状态机数据结构类型 */

struct XML_STATUS_MACHINE {
	/**< 状态码 */
	int   status;

	/**< 状态机处理函数 */
	char *(*callback) (ACL_XML3*, char*);
};

static void xml_parse_check_self_closed(ACL_XML3 *xml)
{
	if ((xml->curr_node->flag & ACL_XML3_F_LEAF) == 0) {
		if (acl_xml3_tag_leaf(xml->curr_node->ltag))
			xml->curr_node->flag |= ACL_XML3_F_LEAF;
	}

	if ((xml->curr_node->flag & ACL_XML3_F_SELF_CL) == 0) {
		if (xml->curr_node->last_ch == '/'
		    || acl_xml3_tag_selfclosed(xml->curr_node->ltag))
		{
			xml->curr_node->flag |= ACL_XML3_F_SELF_CL;
		}
	}
}

static char *xml_parse_next_left_lt(ACL_XML3 *xml, char *data)
{
	SKIP_SPACE(data);
	SKIP_WHILE(*data != '<', data);
	if (*data == 0)
		return data;
	data++;
	xml->curr_node->status = ACL_XML3_S_LLT;
	return data;
}

static char *xml_parse_left_lt(ACL_XML3 *xml, char *data)
{
	xml->curr_node->status = ACL_XML3_S_LCH;
	return data;
}

static char *xml_parse_left_ch(ACL_XML3 *xml, char *data)
{
	int  ch = *data;

	if (ch == '!') {
		xml->curr_node->meta[0] = ch;
		xml->curr_node->status = ACL_XML3_S_LEM;
		data++;
	} else if (ch == '?') {
		xml->curr_node->meta[0] = ch;
		xml->curr_node->flag |= ACL_XML3_F_META_QM;
		xml->curr_node->status = ACL_XML3_S_MTAG;
		data++;
	} else
		xml->curr_node->status = ACL_XML3_S_LTAG;

	return data;
}

static char *xml_parse_left_em(ACL_XML3 *xml, char *data)
{
	if (*data == '-') {
		if (xml->curr_node->meta[1] != '-')
			xml->curr_node->meta[1] = '-';
		else if (xml->curr_node->meta[2] != '-') {
			xml->curr_node->meta[0] = 0;
			xml->curr_node->meta[1] = 0;
			xml->curr_node->meta[2] = 0;
			xml->curr_node->flag |= ACL_XML3_F_META_CM;
			xml->curr_node->status = ACL_XML3_S_MCMT;
		}

		data++;
	} else {
		if (xml->curr_node->meta[1] == '-')
			xml->curr_node->meta[1] = 0;

		xml->curr_node->flag |= ACL_XML3_F_META_EM;
		xml->curr_node->status = ACL_XML3_S_MTAG;
	}

	return data;
}

static char *xml_parse_meta_tag(ACL_XML3 *xml, char *data)
{
	int   ch;

	if (*data == 0)
		return data;

	if (xml->curr_node->ltag == xml->addr)
		xml->curr_node->ltag = data;

	while ((ch = *data) != 0) {
		if (IS_SPACE(ch) || ch == '>') {
			xml->curr_node->ltag_size = data - xml->curr_node->ltag;
			xml->curr_node->status = ACL_XML3_S_MTXT;
			*data++ = 0;
			break;
		}

		data++;
	}

	return data;
}

static char *xml_meta_attr_name(ACL_XML3_ATTR *attr, char *data)
{
	int   ch;
	ACL_XML3 *xml = attr->node->xml;

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	if (attr->name == xml->addr)
		attr->name = data;

	while ((ch = *data) != 0) {
		if (ch == '=') {
			if (attr->name_size == 0)
				attr->name_size = data - attr->name;
			*data++ = 0;
			break;
		}
		if (IS_SPACE(ch)) {
			attr->name_size = data - attr->name;
			*data++ = 0;
		} else
			data++;
	}

	return data;
}

static char *xml_meta_attr_value(ACL_XML3_ATTR *attr, char *data)
{
	ACL_XML3 *xml = attr->node->xml;
	int   ch;

	SKIP_SPACE(data);
	if (IS_QUOTE(*data))
		attr->quote = *data++;

	if (*data == 0)
		return data;

	if (attr->value == xml->addr)
		attr->value = data;

	while ((ch = *data) != 0) {
		if (attr->quote && ch == attr->quote) {
			attr->value_size = data - attr->value;
			*data++ = 0;
			break;
		} else if (IS_SPACE(ch)) {
			attr->value_size = data - attr->value;
			*data++ = 0;
			break;
		}

		data++;
	}

	return data;
}

static void xml_meta_attr(ACL_XML3_NODE *node)
{
	ACL_XML3_ATTR *attr;
	char *ptr;
	int   ch;

	if (node->text == node->xml->addr || *node->text == 0)
		return;

	ptr = node->text;
	SKIP_SPACE(ptr);

	if (*ptr == 0)
		return;

	while ((ch = *ptr) != 0) {
		attr = acl_xml3_attr_alloc(node);
		ptr = xml_meta_attr_name(attr, ptr);
		if (*ptr == 0)
			break;
		ptr = xml_meta_attr_value(attr, ptr);
		if (*ptr == 0)
			break;
	}

	node->text = node->xml->addr;
	node->text_size = 0;
}

static char *xml_parse_meta_text(ACL_XML3 *xml, char *data)
{
	int   ch;

	if (xml->curr_node->text == xml->addr)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	if (xml->curr_node->text == xml->addr)
		xml->curr_node->text = data;

	while ((ch = *data) != 0) {
		if (xml->curr_node->quote) {
			if (ch == xml->curr_node->quote)
				xml->curr_node->quote = 0;
		} else if (IS_QUOTE(ch)) {
			if (xml->curr_node->quote == 0)
				xml->curr_node->quote = ch;
		} else if (ch == '<') {
			xml->curr_node->nlt++;
		} else if (ch != '>') {
			;
		} else if (xml->curr_node->nlt == 0) {
			char *last;

			xml->curr_node->text_size = data - xml->curr_node->text;
			xml->curr_node->status = ACL_XML3_S_MEND;
			*data++ = 0;

			if ((xml->curr_node->flag & ACL_XML3_F_META_QM) == 0)
				break;

			last = data;
			while (last > xml->curr_node->text) {
				if (*last == '?') {
					xml->curr_node->text_size = last -
						xml->curr_node->text;
					*last = 0;
					break;
				}
				last--;
			}
			if (last == xml->curr_node->text)
				break;

			xml_meta_attr(xml->curr_node);
			break;
		} else {
			xml->curr_node->nlt--;
		}

		data++;
	}

	return data;
}

static char *xml_parse_meta_comment(ACL_XML3 *xml, char *data)
{
	int   ch;

	if (xml->curr_node->text == xml->addr)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	if (xml->curr_node->text == xml->addr)
		xml->curr_node->text = data;

	while ((ch = *data) != 0) {
		if (xml->curr_node->quote) {
			if (ch == xml->curr_node->quote)
				xml->curr_node->quote = 0;
		} else if (IS_QUOTE(ch)) {
			if (xml->curr_node->quote == 0)
				xml->curr_node->quote = ch;
		} else if (ch == '<') {
			xml->curr_node->nlt++;
		} else if (ch == '>') {
			if (xml->curr_node->nlt == 0
				&& xml->curr_node->meta[0] == '-'
				&& xml->curr_node->meta[1] == '-')
			{

				xml->curr_node->text_size = data -
					xml->curr_node->text;
				xml->curr_node->status = ACL_XML3_S_MEND;
				*data++ = 0;
				break;
			}

			xml->curr_node->nlt--;
		} else if (xml->curr_node->nlt > 0) {
			;
		} else if (ch == '-') {
			if (xml->curr_node->meta[0] != '-')
				xml->curr_node->meta[0] = '-';
			else if (xml->curr_node->meta[1] != '-')
				xml->curr_node->meta[1] = '-';
		} else {
			if (xml->curr_node->meta[0] == '-')
				xml->curr_node->meta[0] = 0;
			if (xml->curr_node->meta[1] == '-')
				xml->curr_node->meta[1] = 0;
		}

		data++;
	}

	return data;
}

static char *xml_parse_meta_end(ACL_XML3 *xml, char *data)
{
	/* meta 标签是自关闭类型，直接跳至右边 '>' 处理位置 */
	xml->curr_node->status = ACL_XML3_S_RGT;
	return data;
}

static char *xml_parse_left_tag(ACL_XML3 *xml, char *data)
{
	int   ch;

	if (xml->curr_node->ltag == xml->addr)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	if (xml->curr_node->ltag == xml->addr)
		xml->curr_node->ltag = data;

	while ((ch = *data) != 0) {
		if (ch == '>') {
			xml->curr_node->ltag_size = data - xml->curr_node->ltag;
			*data++ = 0;

			xml_parse_check_self_closed(xml);

			if ((xml->curr_node->flag & ACL_XML3_F_SELF_CL)
				&& xml->curr_node->last_ch == '/')
			{
				size_t n = xml->curr_node->ltag_size;
				if (n >= 2)
					xml->curr_node->ltag[n - 2] = 0;
				xml->curr_node->status = ACL_XML3_S_RGT;

			} else
				xml->curr_node->status = ACL_XML3_S_LGT;
			break;
		} else if (IS_SPACE(ch)) {
			xml->curr_node->ltag_size = data - xml->curr_node->ltag;
			xml->curr_node->status = ACL_XML3_S_ATTR;
			xml->curr_node->last_ch = ch;
			*data++ = 0;
			break;
		} else {
			data++;
			xml->curr_node->last_ch = ch;
		}
	}

	return data;
}

static char *xml_parse_attr(ACL_XML3 *xml, char *data)
{
	int   ch;
	ACL_XML3_ATTR *attr = xml->curr_node->curr_attr;

	if (attr == NULL || attr->name == xml->addr) {
		SKIP_SPACE(data);
		SKIP_WHILE(*data == '=', data);
	}

	if (*data == 0)
		return data;

	if (*data == '>') {
		xml_parse_check_self_closed(xml);

		if ((xml->curr_node->flag & ACL_XML3_F_SELF_CL)
			&& xml->curr_node->last_ch == '/')
		{
			xml->curr_node->status = ACL_XML3_S_RGT;
		} else
			xml->curr_node->status = ACL_XML3_S_LGT;

		xml->curr_node->curr_attr = NULL;
		*data++ = 0;

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
		attr = acl_xml3_attr_alloc(xml->curr_node);
		xml->curr_node->curr_attr = attr;
		attr->name = data;
	}

	while ((ch = *data) != 0) {
		xml->curr_node->last_ch = ch;
		if (ch == '=') {
			if (attr->name_size == 0)
				attr->name_size = data - attr->name;
			xml->curr_node->status = ACL_XML3_S_AVAL;
			*data++ = 0;
			break;
		}
		if (IS_SPACE(ch)) {
			if (attr->name_size == 0) {
				attr->name_size = data - attr->name + 1;
				*data = 0;
			}
		}

		data++;
	}

	return data;
}

static char *xml_parse_attr_val(ACL_XML3 *xml, char *data)
{
	int   ch;
	ACL_XML3_ATTR *attr = xml->curr_node->curr_attr;

	if (attr->value == xml->addr && !attr->quote) {
		SKIP_SPACE(data);
		if (IS_QUOTE(*data))
			attr->quote = *data++;
	}

	if (*data == 0)
		return data;

	if (attr->value == xml->addr)
		attr->value = data;

	while ((ch = *data) != 0) {
		if (attr->quote) {
			if (ch == attr->quote) {
				attr->value_size = data - attr->value;
				xml->curr_node->status = ACL_XML3_S_ATTR;
				xml->curr_node->last_ch = ch;
				*data++ = 0;
				break;
			}

			xml->curr_node->last_ch = ch;
		} else if (ch == '>') {
			if (attr->value_size == 0)
				attr->value_size = data - attr->value;
			*data++ = 0;

			xml_parse_check_self_closed(xml);

			if ((xml->curr_node->flag & ACL_XML3_F_SELF_CL)
				&& xml->curr_node->last_ch == '/')
			{
				if (attr->value_size >= 2)
					attr->value[attr->value_size - 2] = 0;
				xml->curr_node->status = ACL_XML3_S_RGT;
			} else
				xml->curr_node->status = ACL_XML3_S_LGT;
			break;
		} else if (IS_SPACE(ch)) {
			attr->value_size = data - attr->value;
			xml->curr_node->status = ACL_XML3_S_ATTR;
			xml->curr_node->last_ch = ch;
			*data++ = 0;
			break;
		} else
			xml->curr_node->last_ch = ch;

		data++;
	}

	/* 当状态发生改变时，则说明属性值已经完毕 */
	if (xml->curr_node->status != ACL_XML3_S_AVAL) {
		/* 将该标签ID号映射至哈希表中，以便于快速查询 */
		if (IS_ID(attr->name) && *attr->value != 0) {
			char *ptr = attr->value;

			/* 防止重复ID被插入现象 */
			if (acl_htable_find(xml->id_table, ptr) == NULL) {
				acl_htable_enter(xml->id_table, ptr, attr);

				/* 当该属性被加入哈希表后才会赋于节点 id */
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

static char *xml_parse_left_gt(ACL_XML3 *xml, char *data)
{
	xml->curr_node->last_ch = 0;
	xml->curr_node->status = ACL_XML3_S_TXT;

	return data;
}

static char *xml_parse_text(ACL_XML3 *xml, char *data)
{
	int   ch;

	if (xml->curr_node->text == xml->addr)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	if (xml->curr_node->text == xml->addr)
		xml->curr_node->text = data;

	while ((ch = *data) != 0) {
		if (ch == '<') {
			xml->curr_node->text_size = data - xml->curr_node->text;
			xml->curr_node->status = ACL_XML3_S_RLT;
			*data++ = 0;
			/* 此处可对文本内容进行 xml 解码 */
			break;
		}

		data++;
	}

	return data;
}

static char *xml_parse_right_lt(ACL_XML3 *xml, char *data)
{
	ACL_XML3_NODE *node;

	SKIP_SPACE(data);
	if (*data == 0)
		return data; 

	if (*data == '/') {  /* get: "</" */
		data++;
		xml->curr_node->status = ACL_XML3_S_RTAG;

		return data;
	} else if ((xml->curr_node->flag & ACL_XML3_F_LEAF)) {
		/* XXX: some error ? */
		xml->curr_node->status = ACL_XML3_S_TXT;

		return data;
	}

	/* 说明遇到了当前节点的子节点 */

	/* 重新设置当前节点状态，以便于其被子节点弹出时可以找到 "</" */
	xml->curr_node->status = ACL_XML3_S_TXT;

	/* 创建新的子节点，并将其加入至当前节点的子节点集合中 */

	node = acl_xml3_node_alloc(xml);
	acl_xml3_node_add_child(xml->curr_node, node);
	node->depth = xml->curr_node->depth + 1;
	if (node->depth > xml->depth)
		xml->depth = node->depth;
	xml->curr_node = node;
	xml->curr_node->status = ACL_XML3_S_LLT;

	return data;
}

/* 因为该父节点其实为叶节点，所以需要更新附属于该伪父节点的
 * 子节点的深度值，都应与该伪父节点相同
 */ 
static void update_children_depth(ACL_XML3_NODE *parent)
{
	ACL_ITER  iter;
	ACL_XML3_NODE *child;

	acl_foreach(iter, parent) {
		child = (ACL_XML3_NODE*) iter.data;
		child->depth = parent->depth;
		update_children_depth(child);
	}
}

/* 查找与右标签相同的父节点 */
static int search_match_node(ACL_XML3 *xml)
{
	ACL_XML3_NODE *parent, *node;
	ACL_ARRAY *nodes = acl_array_create(10);
	ACL_ITER iter;

	parent = acl_xml3_node_parent(xml->curr_node);
	if (parent != xml->root)
		acl_array_append(nodes, xml->curr_node);

	while (parent != xml->root) {
		if (acl_strcasecmp(xml->curr_node->rtag, parent->ltag) == 0) {
			parent->rtag = xml->curr_node->rtag;
			parent->rtag_size = xml->curr_node->rtag_size;
			parent->status = ACL_XML3_S_RGT;
			xml->curr_node = parent;
			break;
		}

		acl_array_append(nodes, parent);

		parent = acl_xml3_node_parent(parent);
	}

	if (parent == xml->root) {
		acl_array_free(nodes, NULL);
		return 0;
	}

	acl_foreach_reverse(iter, nodes) {
		node = (ACL_XML3_NODE*) iter.data;
		acl_ring_detach(&node->node);
		node->flag |= ACL_XML3_F_LEAF;
		node->depth = parent->depth + 1;
		update_children_depth(node);
		acl_xml3_node_add_child(parent, node);
	}

	acl_array_free(nodes, NULL);

	return 1;
}

static char *xml_parse_right_tag(ACL_XML3 *xml, char *data)
{
	int   ch, ret;
	ACL_XML3_NODE *curr_node = xml->curr_node;

	/* after: "</" */

	if (curr_node->rtag == xml->addr)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	if (curr_node->rtag == xml->addr)
		curr_node->rtag = data;

	while ((ch = *data) != 0) {
		if (ch == '>') {
			curr_node->rtag_size = data - curr_node->rtag;
			curr_node->status = ACL_XML3_S_RGT;
			*data++ = 0;
			break;
		}

		if (IS_SPACE(ch)) {
			if (curr_node->rtag_size == 0) {
				curr_node->rtag_size = data - curr_node->rtag;
				*data = 0;
			}
		}

		data++;
	}

	if (curr_node->status != ACL_XML3_S_RGT)
		return data;

	if (acl_strcasecmp(curr_node->ltag, curr_node->rtag) == 0)
		return data;

	if ((xml->flag & ACL_XML3_FLAG_IGNORE_SLASH))
		ret = search_match_node(xml);
	else
		ret = 0;

	if (ret != 0)
		return data;

	/* 如果节点标签名与开始标签名不匹配，
	 * 则需要继续寻找真正的结束标签
	 */ 
	if (curr_node->rtag_size > 0) {
		char *ptr = curr_node->rtag + curr_node->rtag_size - 1;
		if (*ptr == 0)
			*ptr = ' ';
	}

	curr_node->rtag_size = data - curr_node->rtag;

	/* 重新设置当前节点状态，以便于其可以找到 "</" */
	curr_node->status = ACL_XML3_S_TXT;

	return data;
}

static char *xml_parse_right_gt(ACL_XML3 *xml, char *data)
{
	/* 当前节点分析完毕，需要弹出当前节点的父节点继续分析 */
	ACL_XML3_NODE *parent = acl_xml3_node_parent(xml->curr_node);

	if (parent == xml->root) {
		if ((xml->curr_node->flag & ACL_XML3_F_META) == 0)
			xml->root_cnt++;
		xml->curr_node = NULL;
	} else
		xml->curr_node = parent;

	return data;
}

static struct XML_STATUS_MACHINE status_tab[] = {
	{ ACL_XML3_S_NXT,  xml_parse_next_left_lt },
	{ ACL_XML3_S_LLT,  xml_parse_left_lt      },
	{ ACL_XML3_S_LGT,  xml_parse_left_gt      },
	{ ACL_XML3_S_LCH,  xml_parse_left_ch      },
	{ ACL_XML3_S_LEM,  xml_parse_left_em      },
	{ ACL_XML3_S_LTAG, xml_parse_left_tag     },
	{ ACL_XML3_S_RLT,  xml_parse_right_lt     },
	{ ACL_XML3_S_RGT,  xml_parse_right_gt     },
	{ ACL_XML3_S_RTAG, xml_parse_right_tag    },
	{ ACL_XML3_S_ATTR, xml_parse_attr         },
	{ ACL_XML3_S_AVAL, xml_parse_attr_val     },
	{ ACL_XML3_S_TXT,  xml_parse_text         },
	{ ACL_XML3_S_MTAG, xml_parse_meta_tag     },
	{ ACL_XML3_S_MTXT, xml_parse_meta_text    },
	{ ACL_XML3_S_MCMT, xml_parse_meta_comment },
	{ ACL_XML3_S_MEND, xml_parse_meta_end     },
};

char *acl_xml3_update(ACL_XML3 *xml, char *data)
{
	if (data == NULL || *data == 0)
		return data;

	if (!(xml->flag & ACL_XML3_FLAG_MULTI_ROOT) && xml->root_cnt > 0)
		return data;

	/* XML 解析器状态机循环处理过程 */

	while (*data) {
		if (xml->curr_node == NULL) {
			if (!(xml->flag & ACL_XML3_FLAG_MULTI_ROOT)
				&& xml->root_cnt > 0)
			{
				break;
			}

			SKIP_SPACE(data);
			if (*data == 0)
				break;

			xml->curr_node = acl_xml3_node_alloc(xml);
			acl_xml3_node_add_child(xml->root, xml->curr_node);
			xml->curr_node->depth = xml->root->depth + 1;
			if (xml->curr_node->depth > xml->depth)
				xml->depth = xml->curr_node->depth;
		}

		data = status_tab[xml->curr_node->status].callback(xml, data);
	}

	return data;
}
