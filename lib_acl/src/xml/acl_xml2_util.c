#include "StdAfx.h"
#include <stdio.h>

#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_vstream.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_token_tree.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_argv.h"
#include "code/acl_xmlcode.h"
#include "xml/acl_xml2.h"

#endif

static ACL_TOKEN *tag_tree = NULL;

void acl_xml2_tag_init(void)
{
	const char *tag_tab = "input|p, meta|p, link|p, hr|p, br|p";

	if (tag_tree == NULL)
		tag_tree = acl_token_tree_create(tag_tab);
}

void acl_xml2_tag_add(const char *tag)
{
	if (tag_tree) {
		char  buf[256];

		ACL_SAFE_STRNCPY(buf, tag, sizeof(buf));
		acl_lowercase(buf);
		acl_token_tree_add(tag_tree, buf, ACL_TOKEN_F_STOP, NULL);
	}
}

int acl_xml2_tag_selfclosed(const char *tag)
{
	const ACL_TOKEN *token;
	char  buf[256];

	if (tag_tree == NULL)
		return (0);

	ACL_SAFE_STRNCPY(buf, tag, sizeof(buf));
	acl_lowercase(buf);
	token = acl_token_tree_word_match(tag_tree, buf);

	if (token)
		return 1;
	else
		return 0;
}

int acl_xml2_tag_leaf(const char *tag)
{
	if (strcasecmp(tag, "script") == 0)
		return 1;
	return 0;
}

void acl_xml2_free_array(ACL_ARRAY *a)
{
	acl_array_destroy(a, NULL);
}

ACL_XML2_NODE *acl_xml2_getFirstElementByTagName(ACL_XML2 *xml, const char *tag)
{
	ACL_ITER iter;

	acl_foreach(iter, xml) {
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter.data;
		if (strcasecmp(tag, node->ltag) == 0)
			return node;
	}

	return NULL;
}

ACL_ARRAY *acl_xml2_getElementsByTagName(ACL_XML2 *xml, const char *tag)
{
	ACL_ITER iter;
	ACL_ARRAY *a = acl_array_create(10);

	acl_foreach(iter, xml) {
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter.data;
		if (strcasecmp(tag, node->ltag) == 0)
			acl_array_append(a, node);
	}

	if (acl_array_size(a) == 0) {
		acl_array_destroy(a, NULL);
		return NULL;
	}

	return a;
}

ACL_ARRAY *acl_xml2_getElementsByTags(ACL_XML2 *xml, const char *tags)
{
	ACL_ARGV *tokens = acl_argv_split(tags, "/");
	ACL_ARRAY *a, *ret;
	ACL_ITER iter;

	a = acl_xml2_getElementsByTagName(xml, tokens->argv[tokens->argc - 1]);
	if (a == NULL) {
		acl_argv_free(tokens);
		return NULL;
	}

	ret = acl_array_create(acl_array_size(a));

	acl_foreach(iter, a) {
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter.data, *parent = node;
		int   i = tokens->argc - 2;
		while (i >= 0 && (parent = parent->parent) != xml->root) {
			if (strcasecmp(tokens->argv[i], "*") != 0 &&
				strcasecmp(tokens->argv[i], parent->ltag) != 0)
			{
				break;
			}
			i--;
		}
		if (i == -1)
			ret->push_back(ret, node);
	}

	acl_xml2_free_array(a);
	acl_argv_free(tokens);

	if (acl_array_size(ret) == 0) {
		acl_array_free(ret, NULL);
		ret = NULL;
	}
	return ret;
}

ACL_ARRAY *acl_xml2_getElementsByAttr(ACL_XML2 *xml,
	const char *name, const char *value)
{
	ACL_ITER iter;
	ACL_ARRAY *a = acl_array_create(10);

	acl_foreach(iter, xml) {
		ACL_ITER iter2;
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter.data;

		acl_foreach(iter2, node->attr_list) {
			ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter2.data;

			if (strcasecmp(attr->name,  name) == 0
				&& strcasecmp(attr->value, value) == 0)
			{
				acl_array_append(a, node);
				break;
			}
		}
	}

	if (acl_array_size(a) == 0) {
		acl_array_destroy(a, NULL);
		return NULL;
	}

	return a;
}

ACL_ARRAY *acl_xml2_getElementsByName(ACL_XML2 *xml, const char *value)
{
	return acl_xml2_getElementsByAttr(xml, "name", value);
}

ACL_XML2_ATTR *acl_xml2_getAttrById(ACL_XML2 *xml, const char *id)
{
	return acl_htable_find(xml->id_table, id);
}

const char *acl_xml2_getAttrValueById(ACL_XML2 *xml, const char *id)
{
	ACL_XML2_ATTR *attr = acl_xml2_getAttrById(xml, id);

	if (attr == NULL)
		return NULL;

	return attr->value;
}

ACL_XML2_NODE *acl_xml2_getElementById(ACL_XML2 *xml, const char *id)
{
	ACL_XML2_ATTR *attr = acl_xml2_getAttrById(xml, id);

	if (attr == NULL)
		return NULL;
	return attr->node;
}
ACL_XML2_NODE *acl_xml2_getElementMeta(ACL_XML2 *xml, const char *tag)
{
	ACL_ITER iter;
	ACL_XML2_NODE *node;

	acl_foreach(iter, xml) {
		node = (ACL_XML2_NODE*) iter.data;
		if ((node->flag & ACL_XML2_F_META_QM) == 0 || !node->ltag)
			continue;
		if (strcasecmp(tag, node->ltag) == 0)
			return node;
	}

	return NULL;
}

const char *acl_xml2_getEncoding(ACL_XML2 *xml)
{
	ACL_XML2_NODE *node = acl_xml2_getElementMeta(xml, "xml");

	if (node == NULL)
		return NULL;
	return acl_xml2_getElementAttrVal(node, "encoding");
}

const char *acl_xml2_getType(ACL_XML2 *xml)
{
	ACL_XML2_NODE *node = acl_xml2_getElementMeta(xml, "xml-stylesheet");

	if (node == NULL)
		return NULL;
	return acl_xml2_getElementAttrVal(node, "type");
}

ACL_XML2_ATTR *acl_xml2_getElementAttr(ACL_XML2_NODE *node, const char *name)
{
	ACL_ITER iter;

	acl_foreach(iter, node->attr_list) {
		ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter.data;

		if (strcasecmp(attr->name, name) == 0)
			return attr;
	}

	return NULL;
}

const char *acl_xml2_getElementAttrVal(ACL_XML2_NODE *node, const char *name)
{
	ACL_XML2_ATTR *attr = acl_xml2_getElementAttr(node, name);

	if (attr)
		return attr->value;

	return NULL;
}

int acl_xml2_removeElementAttr(ACL_XML2_NODE *node, const char *name)
{
	ACL_XML2_ATTR *attr = acl_xml2_getElementAttr(node, name);

	if (attr == NULL)
		return -1;

	if (acl_array_delete_obj(node->attr_list, attr, NULL) != 0)
		return -1;

	if (node->id == attr->value) {
		acl_htable_delete(node->xml->id_table, attr->value, NULL);
		node->id = NULL;
	}

	return 0;
}

/***************************************************************************/

#define	STR		acl_vstring_str
#define	LEN		ACL_VSTRING_LEN
#define	END(x)		acl_vstring_end((x)->vbuf)
#define	ADD(x, ch)	ACL_VSTRING_ADDCH((x)->vbuf, (ch))
#define	APPEND(x, y)	acl_vstring_strcat((x)->vbuf, (y))
#define	TERM(x)		ACL_VSTRING_TERMINATE((x)->vbuf)
#define	NO_SPACE(x)	acl_vbuf_eof(&((x)->vbuf->vbuf))

void acl_xml2_node_set_text(ACL_XML2_NODE *node, const char *text)
{
	if (text == NULL || *text == 0)
		return;

	node->text = END(node->xml);
	APPEND(node->xml, text);
	node->text_size = END(node->xml) - node->text;
	ADD(node->xml, '\0');
}

ACL_XML2_NODE *acl_xml2_create_node(ACL_XML2 *xml, const char* tag,
	const char* text)
{
	ACL_XML2_NODE *node = acl_xml2_node_alloc(xml);

	acl_assert(tag && *tag);

	node->ltag = END(xml);
	APPEND(xml, tag);
	node->ltag_size = END(xml) - node->ltag;
	ADD(xml, '\0');

	if (text && *text) {
		node->text = END(xml);
		APPEND(xml, text);
		node->text_size = END(xml) - node->text;
		ADD(xml, '\0');
	}

	return node;
}

ACL_XML2_ATTR *acl_xml2_node_add_attr(ACL_XML2_NODE *node, const char *name,
	const char *value)
{
	ACL_XML2_ATTR *attr = acl_xml2_attr_alloc(node);

	acl_assert(name && *name);

	attr->name = END(node->xml);
	APPEND(node->xml, name);
	attr->name_size = END(node->xml) - attr->name;
	ADD(node->xml, '\0');

	if (value && *value) {
		attr->value = END(node->xml);
		APPEND(node->xml, value);
		attr->value_size = END(node->xml) - attr->value;
		ADD(node->xml, '\0');
	}

	return attr;
}

void acl_xml2_node_add_attrs(ACL_XML2_NODE *node, ...)
{
	va_list ap;
	const char *name, *value;

	va_start(ap, node);
	while ((name = va_arg(ap, const char*)) != 0) {
		value = va_arg(ap, const char*);
		acl_assert(value);
		acl_xml2_node_add_attr(node, name, value);
	}
	va_end(ap);
}

ACL_XML2_ATTR *acl_xml2_addElementAttr(ACL_XML2_NODE *node,
	const char *name, const char *value)
{
	ACL_XML2_ATTR *attr = acl_xml2_getElementAttr(node, name);

	if (attr) {
		attr->value = END(node->xml);
		APPEND(node->xml, value);
		attr->value_size = END(node->xml) - attr->value;
		ADD(node->xml, '\0');
		return attr;
	}

	attr = acl_xml2_attr_alloc(node);
	attr->name = END(node->xml);
	APPEND(node->xml, name);
	attr->name_size = END(node->xml) - attr->name;
	ADD(node->xml, '\0');

	attr->value = END(node->xml);
	APPEND(node->xml, value);
	attr->value_size = END(node->xml) - attr->value;
	ADD(node->xml, '\0');

	acl_array_append(node->attr_list, attr);

	return attr;
}

/***************************************************************************/

static void escape_append(ACL_XML2 *xml, const char *in, int quoted)
{
	if (quoted)
		ADD(xml, '"');

	if (in && *in)
		(void) acl_xml_encode(in, xml->vbuf);

	if (quoted)
		ADD(xml, '"');

	TERM(xml);
}

const char *acl_xml2_build(ACL_XML2 *xml)
{
	ACL_XML2_ATTR *attr;
	ACL_XML2_NODE *node;
	ACL_ITER iter1, iter2;
	char *res = END(xml);

	acl_foreach(iter1, xml) {
		if (NO_SPACE(xml))
			break;

		node = (ACL_XML2_NODE*) iter1.data;

		if (ACL_XML2_IS_CDATA(node)) {
			APPEND(xml, "<![CDATA[");
			if (node->text_size > 0)
				APPEND(xml, node->text);
		} else if (ACL_XML2_IS_COMMENT(node)) {
			APPEND(xml, "<!--");
			if (node->text_size > 0)
				APPEND(xml, node->text);
		} else if ((node->flag & ACL_XML2_F_META_QM)) {
			APPEND(xml, "<?");
			if (node->ltag_size > 0)
				APPEND(xml, node->ltag);
		} else if ((node->flag & ACL_XML2_F_META_EM)) {
			APPEND(xml, "<!");
			if (node->ltag_size > 0)
				APPEND(xml, node->ltag);

			ADD(xml, ' ');

			if (node->text_size > 0)
				APPEND(xml, node->text);
		} else {
			ADD(xml, '<');
			if (node->ltag_size > 0)
				APPEND(xml, node->ltag);
		}

		acl_foreach(iter2, node->attr_list) {
			attr = (ACL_XML2_ATTR*) iter2.data;
			ADD(xml, ' ');
			APPEND(xml, attr->name);
			ADD(xml, '=');
			escape_append(xml, attr->value, 1);
		}

		if (acl_ring_size(&node->children) > 0) {
			ADD(xml, '>');

			if (node->text_size > 0)
				escape_append(xml, node->text, 0);

			continue;
		}

		if (ACL_XML2_IS_CDATA(node)) {
			APPEND(xml, "]]>");
		} else if (ACL_XML2_IS_COMMENT(node)) {
			APPEND(xml, "-->");
		} else if (node->flag & ACL_XML2_F_META_QM) {
			APPEND(xml, "?>");
		} else if (node->flag & ACL_XML2_F_META_EM) {
			ADD(xml, '>');
		} else if (node->text_size == 0) {
			APPEND(xml, "></");
			escape_append(xml, node->ltag, 0);
			ADD(xml, '>');
		} else {
			ADD(xml, '>');
			escape_append(xml, node->text, 0);
			APPEND(xml, "</");
			escape_append(xml, node->ltag, 0);
			ADD(xml, '>');
		}

		while (node->parent != node->xml->root
			&& acl_xml2_node_next(node) == NULL)
		{
			APPEND(xml, "</");
			escape_append(xml, node->parent->ltag, 0);
			ADD(xml, '>');
			node = node->parent;
		}
	}

	TERM(xml);

	return res;
}

void acl_xml2_dump(ACL_XML2 *xml, ACL_VSTREAM *fp)
{
	int   i;
	ACL_ITER iter1;
	const char *sep = "\t";

	acl_foreach(iter1, xml) {
		ACL_ITER iter2;
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter1.data;

		for (i = 1; i < node->depth; i++)
			acl_vstream_buffed_fprintf(fp, "%s", sep);

		if (ACL_XML2_IS_COMMENT(node))
			acl_vstream_buffed_fprintf(fp, "comment> text: %s\n",
				node->text);
		else
			acl_vstream_buffed_fprintf(fp, "tag> %s, text: %s\n",
				node->ltag, node->text);

		acl_foreach(iter2, node->attr_list) {
			ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter2.data;

			for (i = 1; i < node->depth; i++)
				acl_vstream_buffed_fprintf(fp, "%s", sep);

			acl_vstream_buffed_fprintf(fp, "%sattr> %s: %s\n",
				sep, attr->name, attr->value);
		}
	}
	acl_vstream_fflush(fp);
}

void acl_xml2_dump2(ACL_XML2 *xml, ACL_VSTRING *buf)
{
	int   i;
	ACL_ITER iter1;
	const char *sep = "\t";

	acl_foreach(iter1, xml) {
		ACL_ITER iter2;
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter1.data;

		for (i = 1; i < node->depth; i++)
			acl_vstring_strcat(buf, sep);

		if (ACL_XML2_IS_COMMENT(node))
			acl_vstring_sprintf_append(buf,
				"comment> text: %s\n", node->text);
		else
			acl_vstring_sprintf_append(buf, "tag> %s, text: %s\n",
				node->ltag, node->text);

		acl_foreach(iter2, node->attr_list) {
			ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter2.data;

			for (i = 1; i < node->depth; i++)
				acl_vstring_strcat(buf, sep);

			acl_vstring_sprintf_append(buf, "%sattr> %s: %s\n",
				sep, attr->name, attr->value);
		}
	}
}
