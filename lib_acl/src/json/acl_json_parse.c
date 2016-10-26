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

/* �����ڵ����ֵ�������ҵ� '{' �� '[' */

static const char *json_obj(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *obj;

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	/* �������� '{}' �ӽڵ� */

	obj = acl_json_node_alloc(json);
	obj->type = ACL_JSON_T_OBJ;
	obj->depth = json->curr_node->depth + 1;
	if (obj->depth > json->depth)
		json->depth = obj->depth;

	/* ���� json �ڵ����ǰ׺�Ĳ�ͬ����¼��ͬ�Ķ����׺ */
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
	/* ����������������ڵ�ĳ�Ա���� */
	ACL_JSON_NODE *member = acl_json_node_alloc(json);

	member->type = ACL_JSON_T_MEMBER;
	member->depth = json->curr_node->depth + 1;
	if (member->depth > json->depth)
		json->depth = member->depth;

	acl_json_node_add_child(json->curr_node, member);

	/* ���ó�Ա������Ϊ��ǰ JSON �����ڵ� */
	json->curr_node = member;
	json->status = ACL_JSON_S_PAIR;

	return data;
}

/* �����ڵ�ı�ǩ���ƣ��ڵ�����û�б�ǩ����Ҷ�ڵ�û�� { } [ ] �ָ��� */

static const char *json_pair(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *parent = acl_json_node_parent(json->curr_node);

	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	acl_assert(parent);

	/* �����ǰ�ַ�Ϊ���ڵ���ҷָ��������ʾ���ڵ���� */
	if (*data == parent->right_ch) {
		data++;  /* ȥ�����ڵ���ҷָ��� */
		if (parent == json->root) {
			/* ������ڵ�������������� json ������� */
			json->finish = 1;
			return data;
		}
		/* �������ڵ� */
		json->curr_node = parent;
		/* ��ѯ���ڵ����һ���ֵܽڵ� */
		json->status = ACL_JSON_S_NEXT;
		return data;
	}

	/* Ϊ '{' �� '[' ʱ˵�������˵�ǰ�ڵ���ӽڵ� */
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

	/* �����ǩ��ǰ�����ţ���¼�¸����� */
	if (IS_QUOTE(*data) && json->curr_node->quote == 0)
		json->curr_node->quote = *data++;

	json->curr_node->type = ACL_JSON_T_PAIR;
	json->status = ACL_JSON_S_TAG;

	return data;
}

/* �����ڵ�ı�ǩ���ƣ��ڵ�����û�б�ǩ����Ҷ�ڵ�û�� { } [ ] �ָ��� */

static const char *json_tag(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *node = json->curr_node;
	int   ch;

	while ((ch = *data) != 0) {
		/* ���ǰ�������ţ�����Ҫ�ҵ���β���� */
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

			/* ��Ϊ˫�ֽں���ʱ����һ���ֽ�Ϊ�ĸ�λΪ 1��
			 * �ڶ����ֽ�Ϊ 92��������ת���ַ���ͬ
			 */
			else if (ch == '\\') {
				/* ���������ֵ����� */
				if (node->part_word) {
					ADDCH(node->ltag, ch);
					node->part_word = 0;
				} else
					node->backslash = 1;
			} else if (ch == node->quote) {
				ACL_JSON_NODE *parent;

				parent = acl_json_node_parent(node);

				acl_assert(parent);

				/* ���������ӽڵ�����Ϊ�������ַ�������� */
				if (parent->left_ch == '[')
					json->status = ACL_JSON_S_NEXT;

				/* ��ǩֵ������������һ����Ҫ�ҵ�ð�� */
				else
					json->status = ACL_JSON_S_COLON;

				/* ���ڷ�����ǩ����������Ҫ�� quote �� 0��
				 * �����ڷ�����ǩֵʱ�����Ը��ø� quote ����,
				 * ������� 0�������ŷ�����ǩֵ����
				 */
				node->quote = 0;
				node->part_word = 0;
				data++;
				break;
			}

			/* �Ƿ���ݺ�������Ϊת��� '\' ����� */
			else if ((json->flag & ACL_JSON_FLAG_PART_WORD)) {
				ADDCH(node->ltag, ch);

				/* ���������ֵ����� */
				if (node->part_word)
					node->part_word = 0;
				else if (ch < 0)
					node->part_word = 1;
			} else {
				ADDCH(node->ltag, ch);
			}
		}

		/* ������ǩ��ǰû�����ŵ���� */

		else if (node->backslash) {
			ADDCH(node->ltag, ch);
			node->backslash = 0;
		}

		/* ��Ϊ˫�ֽں���ʱ����һ���ֽ�Ϊ�ĸ�λΪ 1��
		 * �ڶ����ֽ�Ϊ 92��������ת���ַ���ͬ
		 */
		else if (ch == '\\') {
			/* ���������ֵ����� */
			if (node->part_word) {
				ADDCH(node->ltag, ch);
				node->part_word = 0;
			} else
				node->backslash = 1;
		} else if (IS_SPACE(ch) || ch == ':') {
			/* ��ǩ��������������һ����Ҫ�ҵ�ð�� */
			json->status = ACL_JSON_S_COLON;
			node->part_word = 0;
			break;
		}

		/* �Ƿ���ݺ�������Ϊת��� '\' ����� */
		else if ((json->flag & ACL_JSON_FLAG_PART_WORD)) {
			ADDCH(node->ltag, ch);

			/* ���������ֵ����� */
			if (node->part_word)
				node->part_word = 0;
			else if (ch < 0)
				node->part_word = 1;
		} else {
			ADDCH(node->ltag, ch);
		}
		data++;
	}

	/* �����ǩ���ǿգ�����Ҫ��֤�� 0 ��β */
	if (LEN(node->ltag) > 0)
		ACL_VSTRING_TERMINATE(node->ltag);

	return data;
}

/* һֱ�鵽ð��Ϊֹ��Ȼ���л���������ǩֵ���� */

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

	/* ��һ��������ǩ������Ӧ�ı�ǩֵ���п���Ϊ�ַ�����
	 * Ҳ�п���Ϊ�ӽڵ����
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

	/* ����������� */
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

	return data;
}

static const char *json_element(ACL_JSON *json, const char *data)
{
	/* ���������Ա���� */
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

	/* ���������Ա������Ϊ��ǰ JSON �����ڵ� */
	json->curr_node = element;
	json->status = ACL_JSON_S_VALUE;

	return data;
}

/* ������ǩֵ����ֵ�п����Ǵ��ı�(���ýڵ�ΪҶ�ڵ�)��Ҳ�п�����һ���ӽڵ� */

static const char *json_value(ACL_JSON *json, const char *data)
{
	SKIP_SPACE(data);
	if (*data == 0)
		return data;

	/* Ϊ '{' �� '[' ʱ˵�������˵�ǰ�ڵ���ӽڵ� */
	if (*data == '{') {
		data++;
		json->status = ACL_JSON_S_OBJ;
	} else if (*data == '[') {
		data++;
		json->status = ACL_JSON_S_ARRAY;
	}

	/* ����һ����Щ���ݸ�ʽΪ "xxx: ," �ķ�ʽ */
	else if (*data == ',' || *data == ';') {
		data++;
		/* �л�����ѯ�ýڵ���ֵܽڵ�Ĺ��� */
		json->status = ACL_JSON_S_NEXT;
	}

	/* ˵����ǩ������ı�ǩֵΪ�ַ��������� */
	/* �����ǩֵǰ�����ţ���¼�¸����� */
	else if (IS_QUOTE(*data)) { /* && json->curr_node->quote == 0) { */
		json->curr_node->quote = *data++;
		json->status = ACL_JSON_S_STRING;
	}
	else
		json->status = ACL_JSON_S_STRING;

	json->curr_node->type = ACL_JSON_T_LEAF;
	return data;
}

static const char *json_string(ACL_JSON *json, const char *data)
{
	ACL_JSON_NODE *node = json->curr_node;
	int   ch;

	/* ���ı�����Ϊ 0 ʱ��������Ϊ��δ������Ч���ַ� */

	if (LEN(node->text) == 0) {
		/* �ȹ��˿�ͷû�õĿո� */
		SKIP_SPACE(data);
		if (*data == 0)
			return data;
	}

	/* ˵�����ڵ���Ҷ�ڵ� */

	while ((ch = *data) != 0) {
		/* �����ʼ�����ţ�����Ҫ�Ը�������Ϊ��β�� */
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

			/* ��Ϊ˫�ֽں���ʱ����һ���ֽ�Ϊ�ĸ�λΪ 1��
			 * �ڶ����ֽ��п���Ϊ 92��������ת���ַ���ͬ
			 */
			else if (ch == '\\') {
				/* ���������ֵ���������ǰһ���ֽ���ǰ
				 * ������֣���ǰ��ת���������������
				 */
				if (node->part_word) {
					ADDCH(node->text, ch);
					node->part_word = 0;
				} else
					node->backslash = 1;
			} else if (ch == node->quote) {
				/* �Խڵ��ֵ�����뱣���� quote ֵ���Ա�������
				 * ��ͬ��ֵ���ͣ�bool, null, number, string
				 * node->quote = 0;
				 */

				/* �л�����ѯ�ýڵ���ֵܽڵ�Ĺ��� */
				json->status = ACL_JSON_S_STREND;
				node->part_word = 0;
				data++;
				break;
			}

			/* �Ƿ���ݺ�������Ϊת��� '\' ����� */
			else if ((json->flag & ACL_JSON_FLAG_PART_WORD)) {
				ADDCH(node->text, ch);

				/* ��ǰһ���ֽ�Ϊǰ������֣���ǰ�ֽ�
				 * Ϊ�������֣�����Ϊһ�������ĺ���
				 */
				if (node->part_word)
					node->part_word = 0;

				/* ǰһ���ֽڷ�ǰ��������ҵ�ǰ�ֽڸ�λ
				 * Ϊ 1���������ǰ�ֽ�Ϊǰ�������
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
			/* �л�����ѯ�ýڵ���ֵܽڵ�Ĺ��� */
			json->status = ACL_JSON_S_STREND;
			break;
		}

		/* �Ƿ���ݺ�������Ϊת��� '\' ����� */
		else if ((json->flag & ACL_JSON_FLAG_PART_WORD)) {
			ADDCH(node->text, ch);

			/* ���������ֵ����� */
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

/* ���Է������ڵ����һ���ֵܽڵ㣬�������ҵ��ָ��� ',' �� ';' */

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

	/* ���������ڵ�Ľ��������� json ����������� */
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
		/* ��ѯ���ڵ����һ���ֵܽڵ� */
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

/* ״̬�����ݽṹ���� */

struct JSON_STATUS_MACHINE {
	/* ״̬�� */
	int   status;

	/* ״̬�������� */
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

	/* ����Ƿ��Ѿ�������� */
	if (json->finish)
		return ptr;

	/* json ������״̬��ѭ��������� */

	while (*ptr && !json->finish)
		ptr = status_tab[json->status].callback(json, ptr);

	return ptr;
}

int acl_json_finish(ACL_JSON *json)
{
	return json->finish;
}
