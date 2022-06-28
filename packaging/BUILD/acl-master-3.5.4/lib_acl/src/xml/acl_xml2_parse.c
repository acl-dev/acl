#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_UNIX
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_vbuf.h"
#include "stdlib/acl_vstring.h"
#include "code/acl_xmlcode.h"
#include "xml/acl_xml2.h"

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

#define	STR		acl_vstring_str
#define	LEN		ACL_VSTRING_LEN
#define	END(x)		acl_vstring_end((x)->vbuf)
#define	ADD(x, ch)	ACL_VSTRING_ADDCH((x)->vbuf, (ch))
#define	APPEND(x, y)	acl_vstring_strcat((x)->vbuf, (y))
#define	TERM(x)		ACL_VSTRING_TERMINATE((x)->vbuf)
#define	NO_SPACE(x)	acl_vbuf_eof(&((x)->vbuf->vbuf))

/* 状态机数据结构类型 */

struct XML_STATUS_MACHINE {
	/**< 状态码 */
	int   status;

	/**< 状态机处理函数 */
	const char *(*callback) (ACL_XML2*, const char*);
};

static void xml_parse_check_self_closed(ACL_XML2 *xml)
{
	if ((xml->curr_node->flag & ACL_XML2_F_LEAF) == 0) {
		if (acl_xml2_tag_leaf(xml->curr_node->ltag))
			xml->curr_node->flag |= ACL_XML2_F_LEAF;
	}

	if ((xml->curr_node->flag & ACL_XML2_F_SELF_CL) == 0) {
		if (xml->curr_node->last_ch == '/'
		    || acl_xml2_tag_selfclosed(xml->curr_node->ltag))
		{
			xml->curr_node->flag |= ACL_XML2_F_SELF_CL;
		}
	}
}

static const char *xml_parse_next_left_lt(ACL_XML2 *xml, const char *data)
{
	SKIP_SPACE(data);
	SKIP_WHILE(*data != '<', data);
	if (*data == 0)
		return data;
	data++;
	xml->curr_node->status = ACL_XML2_S_LLT;
	return data;
}

static const char *xml_parse_left_lt(ACL_XML2 *xml, const char *data)
{
	xml->curr_node->status = ACL_XML2_S_LCH;
	return data;
}

static const char *xml_parse_left_ch(ACL_XML2 *xml, const char *data)
{
	int  ch = *data;

	if (ch == '!') {
		xml->curr_node->meta[0] = ch;
		xml->curr_node->status = ACL_XML2_S_LEM;
		data++;
	} else if (ch == '?') {
		xml->curr_node->meta[0] = ch;
		xml->curr_node->flag |= ACL_XML2_F_META_QM;
		xml->curr_node->status = ACL_XML2_S_MTAG;
		data++;
	} else
		xml->curr_node->status = ACL_XML2_S_LTAG;

	return data;
}

static const char *xml_parse_left_em(ACL_XML2 *xml, const char *data)
{
	if (*data == '-') {
		if (xml->curr_node->meta[1] != '-')
			xml->curr_node->meta[1] = '-';
		else if (xml->curr_node->meta[2] != '-') {
			xml->curr_node->meta[0] = 0;
			xml->curr_node->meta[1] = 0;
			xml->curr_node->meta[2] = 0;
			xml->curr_node->flag |= ACL_XML2_F_META_CM;
			xml->curr_node->status = ACL_XML2_S_MCMT;
		}

		data++;
	} else {
		if (xml->curr_node->meta[1] == '-') {
			if (NO_SPACE(xml))
				return data;

			if (xml->curr_node->ltag == xml->dummy)
				xml->curr_node->ltag = END(xml);

			ADD(xml, '-');
			xml->curr_node->meta[1] = 0;
		}

		xml->curr_node->flag |= ACL_XML2_F_META_EM;
		xml->curr_node->status = ACL_XML2_S_MTAG;
	}

	return data;
}

static const char *xml_parse_cdata(ACL_XML2 *xml, const char *data)
{
	ACL_XML2_NODE *curr_node = xml->curr_node;
	int   ch;

	while ((ch = *data) != 0) {
		if (NO_SPACE(xml))
			return data;

		data++;

		if (ch == '>') {
			if (curr_node->meta[0] == ']'
				&& curr_node->meta[1] == ']')
			{
				curr_node->status = ACL_XML2_S_MEND;
				curr_node->text_size = (ssize_t)
					(END(xml) - curr_node->text);
				ADD(xml, '\0');
				return data;
			}
			if (curr_node->meta[0] == ']') {
				ADD(xml, curr_node->meta[0]);
				curr_node->meta[0] = 0;
			}
			if (curr_node->meta[1] == ']') {
				ADD(xml, curr_node->meta[1]);
				curr_node->meta[1] = 0;
			}
		} else if (ch == ']') {
			if (curr_node->meta[0] == ']') {
				if (curr_node->meta[1] == ']') {
					ADD(xml, ']');
				} else
					curr_node->meta[1] = ']';
			} else if (curr_node->meta[1] == ']') {
				curr_node->meta[0] = ']';
				curr_node->meta[1] = 0;
				ADD(xml, ']');
			} else
				curr_node->meta[0] = ']';
		} else if (curr_node->meta[0] == ']') {
			ADD(xml, ']');
			curr_node->meta[0] = 0;

			if (curr_node->meta[1] == ']') {
				ADD(xml, ']');
				curr_node->meta[1] = 0;
			}
		} else {
			ADD(xml, ch);
		}
	}

	TERM(xml);

	return data;
}

#define	CDATA_SIZE	(sizeof("[CDATA[") - 1)
#define	IS_CDATA(x) (*(x) == '[' \
	&& (*(x + 1) == 'C' || *(x + 1) == 'c') \
	&& (*(x + 2) == 'D' || *(x + 2) == 'd') \
	&& (*(x + 3) == 'A' || *(x + 3) == 'a') \
	&& (*(x + 4) == 'T' || *(x + 4) == 't') \
	&& (*(x + 5) == 'A' || *(x + 5) == 't') \
	&& *(x + 6) == '[')

static const char *xml_parse_meta_tag(ACL_XML2 *xml, const char *data)
{
	int   ch;

	if (*data == 0)
		return data;

	if (xml->curr_node->ltag == xml->dummy)
		xml->curr_node->ltag = END(xml);

	while ((ch = *data) != 0) {
		if (END(xml) - xml->curr_node->ltag >= (ssize_t) CDATA_SIZE
			&& IS_CDATA(xml->curr_node->ltag))
		{
			xml->curr_node->ltag_size = (ssize_t)
				(END(xml) - xml->curr_node->ltag);
			ADD(xml, '\0');
			xml->curr_node->text = END(xml);
			xml->curr_node->status = ACL_XML2_S_CDATA;
			xml->curr_node->flag |= ACL_XML2_F_CDATA;
			break;
		} else if (IS_SPACE(ch) || ch == '>') {
			xml->curr_node->ltag_size = (ssize_t)
				(END(xml) - xml->curr_node->ltag);
			if (NO_SPACE(xml))
				return data;
			ADD(xml, '\0');
			data++;
			xml->curr_node->status = ACL_XML2_S_MTXT;
			break;
		}

		if (NO_SPACE(xml))
			return data;
		ADD(xml, ch);
		data++;
	}

	return data;
}

static char *xml_meta_attr_name(ACL_XML2_ATTR *attr, char *data)
{
	int   ch;

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	if (attr->name == attr->node->xml->dummy)
		attr->name = data;

	while ((ch = *data) != 0) {
		if (ch == '=') {
			if (attr->name_size == 0)
				attr->name_size = (ssize_t) (data - attr->name);
			*data++ = 0;
			break;
		}
		if (IS_SPACE(ch)) {
			attr->name_size = (ssize_t) (data - attr->name);
			*data++ = 0;
		} else
			data++;
	}

	return data;
}

static char *xml_meta_attr_value(ACL_XML2_ATTR *attr, char *data)
{
	ACL_XML2 *xml = attr->node->xml;
	int   ch;

	SKIP_SPACE(data);
	if (IS_QUOTE(*data))
		attr->quote = *data++;

	if (*data == 0)
		return data;

	if (attr->value == xml->dummy)
		attr->value = data;

	while ((ch = *data) != 0) {
		if (attr->quote && ch == attr->quote) {
			attr->value_size = (ssize_t) (data - attr->value);
			*data++ = 0;
			break;
		} else if (IS_SPACE(ch)) {
			attr->value_size = (ssize_t) (data - attr->value);
			*data++ = 0;
			break;
		}

		data++;
	}

	if ((xml->flag & ACL_XML2_FLAG_XML_DECODE) && attr->value_size > 0) {
		const char *ptr = attr->value;

		attr->value = END(xml);
		(void) acl_xml_decode(ptr, xml->vbuf);
		attr->value_size = (ssize_t) (END(xml) - attr->value);
		ADD(xml, '\0');  /* skip one byte */
	}

	return data;
}

static void xml_meta_attr(ACL_XML2_NODE *node)
{
	ACL_XML2_ATTR *attr;
	char *ptr;
	int   ch;

	if (node->text == node->xml->dummy || *node->text == 0)
		return;

	ptr = node->text;
	SKIP_SPACE(ptr);

	if (*ptr == 0)
		return;

	while ((ch = *ptr) != 0) {
		attr = acl_xml2_attr_alloc(node);
		ptr = xml_meta_attr_name(attr, ptr);
		if (*ptr == 0)
			break;
		ptr = xml_meta_attr_value(attr, ptr);
		if (*ptr == 0)
			break;
	}

	node->text = node->xml->dummy;
	node->text_size = 0;
}

static const char *xml_parse_meta_text(ACL_XML2 *xml, const char *data)
{
	int   ch;

	if (xml->curr_node->text == xml->dummy)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	if (xml->curr_node->text == xml->dummy)
		xml->curr_node->text = END(xml);

	while ((ch = *data) != 0) {
		if (xml->curr_node->quote) {
			if (NO_SPACE(xml))
				return data;
			if (ch == xml->curr_node->quote)
				xml->curr_node->quote = 0;
			ADD(xml, ch);
		} else if (IS_QUOTE(ch)) {
			if (NO_SPACE(xml))
				return data;
			if (xml->curr_node->quote == 0)
				xml->curr_node->quote = ch;
			ADD(xml, ch);
		} else if (ch == '<') {
			if (NO_SPACE(xml))
				return data;
			xml->curr_node->nlt++;
			ADD(xml, ch);
		} else if (ch != '>') {
			if (NO_SPACE(xml))
				return data;
			ADD(xml, ch);
		} else if (xml->curr_node->nlt == 0) {
			char *last;

			if (NO_SPACE(xml))
				return data;

			xml->curr_node->text_size = (ssize_t)
				(END(xml) - xml->curr_node->text);
			xml->curr_node->status = ACL_XML2_S_MEND;

			ADD(xml, '\0');
			data++;

			if ((xml->curr_node->flag & ACL_XML2_F_META_QM) == 0)
				break;

			last = END(xml) - 1;
			while (last > xml->curr_node->text) {
				if (*last == '?') {
					*last = 0;
					xml->curr_node->text_size = (ssize_t)
						(last - xml->curr_node->text);
					break;
				}
				last--;
			}
			if (last == xml->curr_node->text)
				break;

			xml_meta_attr(xml->curr_node);
			break;
		} else {
			if (NO_SPACE(xml))
				return data;
			xml->curr_node->nlt--;
			ADD(xml, ch);
		}

		data++;
	}

	if (xml->curr_node->status == ACL_XML2_S_MEND
		&& (xml->flag & ACL_XML2_FLAG_XML_DECODE)
		&& xml->curr_node->text_size > 0 && !NO_SPACE(xml))
	{
		const char *txt = xml->curr_node->text;

		xml->curr_node->text = END(xml);
		(void) acl_xml_decode(txt, xml->vbuf);
		xml->curr_node->text_size = (ssize_t) 
			(END(xml) - xml->curr_node->text);
		ADD(xml, '\0');  /* skip one byte */
	}

	return data;
}

static const char *xml_parse_meta_comment(ACL_XML2 *xml, const char *data)
{
	int   ch;

	if (xml->curr_node->text == xml->dummy)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	if (xml->curr_node->text == xml->dummy)
		xml->curr_node->text = END(xml);

	while ((ch = *data) != 0) {
		if (xml->curr_node->quote) {
			if (NO_SPACE(xml))
				return data;
			if (ch == xml->curr_node->quote)
				xml->curr_node->quote = 0;
			else
				ADD(xml, ch);
		} else if (IS_QUOTE(ch)) {
			if (NO_SPACE(xml))
				return data;
			if (xml->curr_node->quote == 0)
				xml->curr_node->quote = ch;
			else
				ADD(xml, ch);
		} else if (ch == '<') {
			if (NO_SPACE(xml))
				return data;
			xml->curr_node->nlt++;
			ADD(xml, ch);
		} else if (ch == '>') {
			if (xml->curr_node->nlt == 0
				&& xml->curr_node->meta[0] == '-'
				&& xml->curr_node->meta[1] == '-')
			{
				if (NO_SPACE(xml))
					return data;

				data++;
				xml->curr_node->text_size = (ssize_t)
					(END(xml) - xml->curr_node->text);
				ADD(xml, '\0');
				xml->curr_node->status = ACL_XML2_S_MEND;
				break;
			}

			xml->curr_node->nlt--;

			if (NO_SPACE(xml))
				return data;
			ADD(xml, ch);
		} else if (xml->curr_node->nlt > 0) {
			if (NO_SPACE(xml))
				return data;
			ADD(xml, ch);
		} else if (ch == '-') {
			if (xml->curr_node->meta[0] != '-')
				xml->curr_node->meta[0] = '-';
			else if (xml->curr_node->meta[1] != '-')
				xml->curr_node->meta[1] = '-';
		} else {
			if (xml->curr_node->meta[0] == '-') {
				if (NO_SPACE(xml))
					return data;
				ADD(xml, '-');
				xml->curr_node->meta[0] = 0;
			}
			if (xml->curr_node->meta[1] == '-') {
				if (NO_SPACE(xml))
					return data;
				ADD(xml, '-');
				xml->curr_node->meta[1] = 0;
			}

			if (NO_SPACE(xml))
				return data;
			ADD(xml, ch);
		}

		data++;
	}

	if (xml->curr_node->status == ACL_XML2_S_MEND
		&& (xml->flag & ACL_XML2_FLAG_XML_DECODE)
		&& xml->curr_node->text_size > 0
		&& !NO_SPACE(xml))
	{
		const char *txt = xml->curr_node->text;

		xml->curr_node->text = END(xml);
		(void) acl_xml_decode(txt, xml->vbuf);
		xml->curr_node->text_size = (ssize_t)
			(END(xml) - xml->curr_node->text);
		ADD(xml, '\0');  /* skip one byte */
	}

	return data;
}

static const char *xml_parse_meta_end(ACL_XML2 *xml, const char *data)
{
	/* meta 标签是自关闭类型，直接跳至右边 '>' 处理位置 */
	xml->curr_node->status = ACL_XML2_S_RGT;
	return data;
}

static const char *xml_parse_left_tag(ACL_XML2 *xml, const char *data)
{
	int   ch;

	if (xml->curr_node->ltag == xml->dummy)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	if (xml->curr_node->ltag == xml->dummy)
		xml->curr_node->ltag = END(xml);

	while ((ch = *data) != 0) {
		if (ch == '>') {
			if (NO_SPACE(xml))
				return data;
			xml->curr_node->ltag_size = (ssize_t)
				(END(xml) - xml->curr_node->ltag);
			ADD(xml, '\0');
			data++;

			xml_parse_check_self_closed(xml);

			if ((xml->curr_node->flag & ACL_XML2_F_SELF_CL)
				&& xml->curr_node->last_ch == '/')
			{
				if (xml->curr_node->ltag_size > 0) {
					size_t n;

					xml->curr_node->ltag_size--;
					n = xml->curr_node->ltag_size;
					xml->curr_node->ltag[n] = 0;
				}
				xml->curr_node->status = ACL_XML2_S_RGT;
			} else
				xml->curr_node->status = ACL_XML2_S_LGT;
			break;
		} else if (IS_SPACE(ch)) {
			if (NO_SPACE(xml))
				return data;
			data++;
			xml->curr_node->ltag_size = (ssize_t)
				(END(xml) - xml->curr_node->ltag);
			ADD(xml, '\0');
			xml->curr_node->status = ACL_XML2_S_ATTR;
			xml->curr_node->last_ch = ch;
			break;
		} else {
			if (NO_SPACE(xml))
				return data;
			data++;
			ADD(xml, ch);
			xml->curr_node->last_ch = ch;
		}
	}

	return data;
}

static const char *xml_parse_attr(ACL_XML2 *xml, const char *data)
{
	int   ch;
	ACL_XML2_ATTR *attr = xml->curr_node->curr_attr;

	if (attr == NULL || attr->name == xml->dummy) {
		SKIP_SPACE(data);
		SKIP_WHILE(*data == '=', data);
	}

	if (*data == 0)
		return data;

	if (*data == '>') {
		xml_parse_check_self_closed(xml);

		if ((xml->curr_node->flag & ACL_XML2_F_SELF_CL)
			&& xml->curr_node->last_ch == '/')
		{
			xml->curr_node->status = ACL_XML2_S_RGT;
		} else
			xml->curr_node->status = ACL_XML2_S_LGT;

		xml->curr_node->curr_attr = NULL;
		if (NO_SPACE(xml))
			return data;
		data++;
		ADD(xml, '\0');

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
		attr = acl_xml2_attr_alloc(xml->curr_node);
		xml->curr_node->curr_attr = attr;
		attr->name = END(xml);
	}

	while ((ch = *data) != 0) {
		xml->curr_node->last_ch = ch;
		if (ch == '=') {
			if (NO_SPACE(xml))
				return data;
			data++;
			attr->name_size = (ssize_t) (END(xml) - attr->name);
			ADD(xml, '\0');
			xml->curr_node->status = ACL_XML2_S_AVAL;
			break;
		}
		if (!IS_SPACE(ch)) {
			if (NO_SPACE(xml))
				return data;
			ADD(xml, ch);
		}

		data++;
	}

	return data;
}

static const char *xml_parse_attr_val(ACL_XML2 *xml, const char *data)
{
	int   ch;
	ACL_XML2_ATTR *attr = xml->curr_node->curr_attr;

	if (attr->value == xml->dummy && !attr->quote) {
		SKIP_SPACE(data);
		if (IS_QUOTE(*data))
			attr->quote = *data++;
	}

	if (*data == 0)
		return data;

	if (attr->value == xml->dummy)
		attr->value = END(xml);

	while ((ch = *data) != 0) {
		if (attr->quote) {
			if (ch == attr->quote) {
				if (NO_SPACE(xml))
					return data;
				data++;
				attr->value_size = (ssize_t)
					(END(xml) - attr->value);
				ADD(xml, '\0');
				xml->curr_node->status = ACL_XML2_S_ATTR;
				xml->curr_node->last_ch = ch;
				break;
			}

			if (NO_SPACE(xml))
				return data;
			ADD(xml, ch);
			xml->curr_node->last_ch = ch;
		} else if (ch == '>') {
			if (NO_SPACE(xml))
				return data;
			data++;
			attr->value_size = (ssize_t) (END(xml) - attr->value);
			ADD(xml, '\0');

			xml_parse_check_self_closed(xml);

			if ((xml->curr_node->flag & ACL_XML2_F_SELF_CL)
				&& xml->curr_node->last_ch == '/')
			{
				if (--attr->value_size >= 0)
					attr->value[attr->value_size] = 0;
				xml->curr_node->status = ACL_XML2_S_RGT;
			} else
				xml->curr_node->status = ACL_XML2_S_LGT;
			break;
		} else if (IS_SPACE(ch)) {
			if (NO_SPACE(xml))
				return data;
			data++;
			attr->value_size = (ssize_t) (END(xml) - attr->value);
			ADD(xml, '\0');
			xml->curr_node->status = ACL_XML2_S_ATTR;
			xml->curr_node->last_ch = ch;
			break;
		} else {
			if (NO_SPACE(xml))
				return data;
			ADD(xml, ch);
			xml->curr_node->last_ch = ch;
		}

		data++;
	}

	/* 说明属性值还未解析完，需要继续解析 */
	if (xml->curr_node->status == ACL_XML2_S_AVAL)
		return data;

	/* 当状态发生改变时，则说明属性值已经完毕 */

	if ((xml->flag & ACL_XML2_FLAG_XML_DECODE) && attr->value_size > 1) {
		const char *val = attr->value;

		attr->value = END(xml);
		(void) acl_xml_decode(val, xml->vbuf);
		attr->value_size = (ssize_t) (END(xml) - attr->value);
		ADD(xml, '\0');  /* skip one byte */
	}

	/* 将该标签ID号映射至哈希表中，以便于快速查询 */
	if (IS_ID(attr->name) && *attr->value != 0) {
		const char *ptr = attr->value;

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

	return data;
}

static const char *xml_parse_left_gt(ACL_XML2 *xml, const char *data)
{
	xml->curr_node->last_ch = 0;
	xml->curr_node->status = ACL_XML2_S_TXT;

	return data;
}

static const char *xml_parse_text(ACL_XML2 *xml, const char *data)
{
	int   ch;

	if (xml->curr_node->text == xml->dummy)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	if (xml->curr_node->text == xml->dummy)
		xml->curr_node->text = END(xml);

	while ((ch = *data) != 0) {
		if (ch == '<') {
			if (NO_SPACE(xml))
				return data;

			data++;
			xml->curr_node->text_size = (ssize_t)
				(END(xml) - xml->curr_node->text);
			ADD(xml, '\0');
			xml->curr_node->status = ACL_XML2_S_RLT;
			break;
		}

		if (NO_SPACE(xml))
			return data;

		data++;
		ADD(xml, ch);
	}

	if (xml->curr_node->status == ACL_XML2_S_RLT
		&& (xml->flag & ACL_XML2_FLAG_XML_DECODE)
		&& xml->curr_node->text_size > 1
		&& !NO_SPACE(xml))
	{
		char *txt = xml->curr_node->text;

		xml->curr_node->text = END(xml);
		(void) acl_xml_decode(txt, xml->vbuf);
		xml->curr_node->text_size = (ssize_t)
			(END(xml) - xml->curr_node->text);
		txt = END(xml) - 1;

		while (txt >= xml->curr_node->text && IS_SPACE(*txt)) {
			*txt-- = 0;
			xml->curr_node->text_size--;
		}

		ADD(xml, '\0');
	}

	return data;
}

static const char *xml_parse_right_lt(ACL_XML2 *xml, const char *data)
{
	ACL_XML2_NODE *node;

	SKIP_SPACE(data);
	if (*data == 0)
		return data; 

	if (*data == '/') {  /* get: "</" */
		data++;
		xml->curr_node->status = ACL_XML2_S_RTAG;

		return data;
	} else if ((xml->curr_node->flag & ACL_XML2_F_LEAF)) {
		/* XXX: some error ? */
		if (NO_SPACE(xml))
			return data;
		ADD(xml, '<');
		if (NO_SPACE(xml))
			return data;
		ADD(xml, *data);
		data++;
		xml->curr_node->status = ACL_XML2_S_TXT;

		return data;
	}

	/* 说明遇到了当前节点的子节点 */

	/* 重新设置当前节点状态，以便于其被子节点弹出时可以找到 "</" */
	xml->curr_node->status = ACL_XML2_S_TXT;

	/* 创建新的子节点，并将其加入至当前节点的子节点集合中 */

	node = acl_xml2_node_alloc(xml);
	acl_xml2_node_add_child(xml->curr_node, node);
	node->depth = xml->curr_node->depth + 1;
	if (node->depth > xml->depth)
		xml->depth = node->depth;
	xml->curr_node = node;
	xml->curr_node->status = ACL_XML2_S_LLT;

	return data;
}

/* 因为该父节点其实为叶节点，所以需要更新附属于该伪父节点的
 * 子节点的深度值，都应与该伪父节点相同
 */ 
static void update_children_depth(ACL_XML2_NODE *parent)
{
	ACL_ITER  iter;
	ACL_XML2_NODE *child;

	acl_foreach(iter, parent) {
		child = (ACL_XML2_NODE*) iter.data;
		child->depth = parent->depth;
		update_children_depth(child);
	}
}

/* 查找与右标签相同的父节点 */
static int search_match_node(ACL_XML2 *xml)
{
	ACL_XML2_NODE *parent, *node;
	ACL_ARRAY *nodes = acl_array_create(10);
	ACL_ITER iter;

	parent = acl_xml2_node_parent(xml->curr_node);
	if (parent != xml->root)
		acl_array_append(nodes, xml->curr_node);

	while (parent != xml->root) {
		if (acl_strcasecmp(xml->curr_node->rtag, parent->ltag) == 0) {
			parent->rtag = END(xml);
			APPEND(xml, xml->curr_node->rtag);
			parent->status = ACL_XML2_S_RGT;
			xml->curr_node = parent;
			break;
		}

		acl_array_append(nodes, parent);

		parent = acl_xml2_node_parent(parent);
	}

	if (parent == xml->root) {
		acl_array_free(nodes, NULL);
		return 0;
	}

	acl_foreach_reverse(iter, nodes) {
		node = (ACL_XML2_NODE*) iter.data;
		acl_ring_detach(&node->node);
		node->flag |= ACL_XML2_F_LEAF;
		node->depth = parent->depth + 1;
		update_children_depth(node);
		acl_xml2_node_add_child(parent, node);
	}

	acl_array_free(nodes, NULL);

	return 1;
}

static const char *xml_parse_right_tag(ACL_XML2 *xml, const char *data)
{
	int   ch;
	ACL_XML2_NODE *curr_node = xml->curr_node;

	/* after: "</" */

	if (curr_node->rtag == xml->dummy)
		SKIP_SPACE(data);

	if (*data == 0)
		return data;

	if (curr_node->rtag == xml->dummy)
		curr_node->rtag = END(xml);

	while ((ch = *data) != 0) {
		if (ch == '>') {
			if (NO_SPACE(xml))
				return data;
			data++;
			curr_node->rtag_size = (ssize_t)
				(END(xml) - curr_node->rtag);
			ADD(xml, '\0');
			curr_node->status = ACL_XML2_S_RGT;
			break;
		}

		if (!IS_SPACE(ch)) {
			if (NO_SPACE(xml))
				return data;
			ADD(xml, ch);
		}

		data++;
	}

	if (curr_node->status != ACL_XML2_S_RGT)
		return data;

	if (acl_strcasecmp(curr_node->ltag, curr_node->rtag) != 0) {
		int   ret;

		if ((xml->flag & ACL_XML2_FLAG_IGNORE_SLASH))
			ret = search_match_node(xml);
		else
			ret = 0;

		if (ret == 0) {
			/* 如果节点标签名与开始标签名不匹配，
			 * 则需要继续寻找真正的结束标签
			 */ 
			curr_node->text = END(xml);
			APPEND(xml, curr_node->rtag);

			/* 重新设置当前节点状态，以便于其可以找到 "</" */
			curr_node->status = ACL_XML2_S_TXT;
		}
	}

	return data;
}

static const char *xml_parse_right_gt(ACL_XML2 *xml, const char *data)
{
	/* 当前节点分析完毕，需要弹出当前节点的父节点继续分析 */
	ACL_XML2_NODE *parent = acl_xml2_node_parent(xml->curr_node);

	if (parent == xml->root) {
		if ((xml->curr_node->flag & ACL_XML2_F_META) == 0)
			xml->root_cnt++;
		xml->curr_node = NULL;
	} else
		xml->curr_node = parent;

	return data;
}

static struct XML_STATUS_MACHINE status_tab[] = {
	{ ACL_XML2_S_NXT,   xml_parse_next_left_lt },
	{ ACL_XML2_S_LLT,   xml_parse_left_lt      },
	{ ACL_XML2_S_LGT,   xml_parse_left_gt      },
	{ ACL_XML2_S_LCH,   xml_parse_left_ch      },
	{ ACL_XML2_S_LEM,   xml_parse_left_em      },
	{ ACL_XML2_S_LTAG,  xml_parse_left_tag     },
	{ ACL_XML2_S_RLT,   xml_parse_right_lt     },
	{ ACL_XML2_S_RGT,   xml_parse_right_gt     },
	{ ACL_XML2_S_RTAG,  xml_parse_right_tag    },
	{ ACL_XML2_S_ATTR,  xml_parse_attr         },
	{ ACL_XML2_S_AVAL,  xml_parse_attr_val     },
	{ ACL_XML2_S_TXT,   xml_parse_text         },
	{ ACL_XML2_S_MTAG,  xml_parse_meta_tag     },
	{ ACL_XML2_S_MTXT,  xml_parse_meta_text    },
	{ ACL_XML2_S_MCMT,  xml_parse_meta_comment },
	{ ACL_XML2_S_MEND,  xml_parse_meta_end     },
	{ ACL_XML2_S_CDATA, xml_parse_cdata        },
};

const char *acl_xml2_update(ACL_XML2 *xml, const char *data)
{
	const char *myname = "acl_xml2_update";

	if (data == NULL)
		return "";
	else if (*data == 0)
		return data;

	if (!(xml->flag & ACL_XML2_FLAG_MULTI_ROOT) && xml->root_cnt > 0)
		return data;

	/* XML 解析器状态机循环处理过程 */

	while (*data) {
		if (xml->curr_node == NULL) {
			if (!(xml->flag & ACL_XML2_FLAG_MULTI_ROOT)
				&& xml->root_cnt > 0)
			{
				break;
			}

			SKIP_SPACE(data);
			if (*data == 0)
				break;

			xml->curr_node = acl_xml2_node_alloc(xml);
			acl_xml2_node_add_child(xml->root, xml->curr_node);
			xml->curr_node->depth = xml->root->depth + 1;
			if (xml->curr_node->depth > xml->depth)
				xml->depth = xml->curr_node->depth;
		}

		data = status_tab[xml->curr_node->status].callback(xml, data);

		if (NO_SPACE(xml)) {
			acl_msg_warn("%s(%d), %s: space not enougth!",
				__FILE__, __LINE__, myname);
			break;
		}
	}

	return data;
}
