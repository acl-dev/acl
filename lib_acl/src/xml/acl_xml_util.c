#include "StdAfx.h"
#include <stdio.h>
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_vstream.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_token_tree.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_msg.h"
#include "code/acl_xmlcode.h"
#include "xml/acl_xml.h"

#endif

#define STR	acl_vstring_str
#define LEN	ACL_VSTRING_LEN

static ACL_TOKEN *tag_tree = NULL;

void acl_xml_tag_init(void)
{
	const char *tag_tab = "input|p, meta|p, link|p, hr|p, br|p";

	if (tag_tree == NULL)
		tag_tree = acl_token_tree_create(tag_tab);
}

void acl_xml_tag_add(const char *tag)
{
	if (tag_tree) {
		char  buf[256];

		ACL_SAFE_STRNCPY(buf, tag, sizeof(buf));
		acl_lowercase(buf);
		acl_token_tree_add(tag_tree, buf, ACL_TOKEN_F_STOP, NULL);
	}
}

int acl_xml_tag_selfclosed(const char *tag)
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

int acl_xml_tag_leaf(const char *tag)
{
	if (strcasecmp(tag, "script") == 0)
		return 1;
	return 0;
}

void acl_xml_free_array(ACL_ARRAY *a)
{
	acl_array_destroy(a, NULL);
}

ACL_XML_NODE *acl_xml_getFirstElementByTagName(ACL_XML *xml, const char *tag)
{
	ACL_ITER iter;

	acl_foreach(iter, xml) {
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter.data;
		if (strcasecmp(tag, STR(node->ltag)) == 0)
			return node;
	}

	return NULL;
}

ACL_ARRAY *acl_xml_getElementsByTagName(ACL_XML *xml, const char *tag)
{
	ACL_ITER iter;
	ACL_ARRAY *a = acl_array_create(10);

	acl_foreach(iter, xml) {
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter.data;
		if (strcasecmp(tag, STR(node->ltag)) == 0)
			acl_array_append(a, node);
	}

	if (acl_array_size(a) == 0) {
		acl_array_destroy(a, NULL);
		return NULL;
	}

	return a;
}

ACL_ARRAY *acl_xml_getElementsByTags(ACL_XML *xml, const char *tags)
{
	ACL_ARGV *tokens = acl_argv_split(tags, "/");
	ACL_ARRAY *a, *ret;
	ACL_ITER iter;

	a = acl_xml_getElementsByTagName(xml, tokens->argv[tokens->argc - 1]);
	if (a == NULL) {
		acl_argv_free(tokens);
		return NULL;
	}

	ret = acl_array_create(acl_array_size(a));

#define NE strcasecmp

	acl_foreach(iter, a) {
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter.data, *parent = node;
		int   i = tokens->argc - 2;
		while (i >= 0 && (parent = parent->parent) != xml->root) {
			if (strcasecmp(tokens->argv[i], "*") != 0
				&& NE(tokens->argv[i], STR(parent->ltag)))
			{
				break;
			}
			i--;
		}
		if (i == -1)
			ret->push_back(ret, node);
	}

	acl_xml_free_array(a);
	acl_argv_free(tokens);

	if (acl_array_size(ret) == 0) {
		acl_array_free(ret, NULL);
		ret = NULL;
	}
	return ret;
}

ACL_ARRAY *acl_xml_getElementsByAttr(ACL_XML *xml,
	const char *name, const char *value)
{
	ACL_ITER iter;
	ACL_ARRAY *a = acl_array_create(10);

	acl_foreach(iter, xml) {
		ACL_ITER iter2;
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter.data;

		acl_foreach(iter2, node->attr_list) {
			ACL_XML_ATTR *attr = (ACL_XML_ATTR*) iter2.data;

			if (strcasecmp(STR(attr->name),  name) == 0
				&& strcasecmp(STR(attr->value), value) == 0)
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

ACL_ARRAY *acl_xml_getElementsByName(ACL_XML *xml, const char *value)
{
	return acl_xml_getElementsByAttr(xml, "name", value);
}

ACL_XML_ATTR *acl_xml_getAttrById(ACL_XML *xml, const char *id)
{
	return acl_htable_find(xml->id_table, id);
}

const char *acl_xml_getAttrValueById(ACL_XML *xml, const char *id)
{
	ACL_XML_ATTR *attr = acl_xml_getAttrById(xml, id);

	if (attr == NULL)
		return NULL;

	return STR(attr->value);
}

ACL_XML_NODE *acl_xml_getElementById(ACL_XML *xml, const char *id)
{
	ACL_XML_ATTR *attr = acl_xml_getAttrById(xml, id);

	if (attr == NULL)
		return NULL;
	return attr->node;
}
ACL_XML_NODE *acl_xml_getElementMeta(ACL_XML *xml, const char *tag)
{
	ACL_ITER iter;
	ACL_XML_NODE *node;

	acl_foreach(iter, xml) {
		node = (ACL_XML_NODE*) iter.data;
		if ((node->flag & ACL_XML_F_META_QM) == 0 || !node->ltag)
			continue;
		if (strcasecmp(tag, STR(node->ltag)) == 0)
			return node;
	}

	return NULL;
}

const char *acl_xml_getEncoding(ACL_XML *xml)
{
	ACL_XML_NODE *node = acl_xml_getElementMeta(xml, "xml");

	if (node == NULL)
		return NULL;
	return acl_xml_getElementAttrVal(node, "encoding");
}

const char *acl_xml_getType(ACL_XML *xml)
{
	ACL_XML_NODE *node = acl_xml_getElementMeta(xml, "xml-stylesheet");

	if (node == NULL)
		return NULL;
	return acl_xml_getElementAttrVal(node, "type");
}

ACL_XML_ATTR *acl_xml_getElementAttr(ACL_XML_NODE *node, const char *name)
{
	ACL_ITER iter;

	acl_foreach(iter, node->attr_list) {
		ACL_XML_ATTR *attr = (ACL_XML_ATTR*) iter.data;

		if (strcasecmp(STR(attr->name), name) == 0)
			return attr;
	}

	return NULL;
}

const char *acl_xml_getElementAttrVal(ACL_XML_NODE *node, const char *name)
{
	ACL_XML_ATTR *attr = acl_xml_getElementAttr(node, name);

	if (attr)
		return STR(attr->value);

	return NULL;
}

int acl_xml_removeElementAttr(ACL_XML_NODE *node, const char *name)
{
	ACL_XML_ATTR *attr = acl_xml_getElementAttr(node, name);

	if (attr == NULL)
		return -1;

	if (acl_array_delete_obj(node->attr_list, attr, NULL) != 0)
		return -1;

	if (node->id == attr->value) {
		acl_htable_delete(node->xml->id_table, STR(attr->value), NULL);
		node->id = NULL;
	}

	return 0;
}

/***************************************************************************/

static void escape_copy(ACL_VSTRING *buf, const char *src, ACL_XML *xml)
{
	ACL_VSTRING_RESET(buf);

	if (src && *src) {
		if ((xml->flag & ACL_XML_FLAG_XML_ENCODE) != 0)
			acl_xml_encode(src, buf);
		else
			acl_vstring_strcpy(buf, src);
	}
}

static void escape_append(ACL_VSTRING *buf, const char *src, ACL_XML *xml)
{
	if (src && *src) {
		if ((xml->flag & ACL_XML_FLAG_XML_ENCODE) != 0)
			acl_xml_encode(src, buf);
		else
			acl_vstring_strcat(buf, src);
	}
}

/***************************************************************************/

ACL_XML_ATTR *acl_xml_addElementAttr(ACL_XML_NODE *node,
	const char *name, const char *value)
{
	ACL_XML_ATTR *attr = acl_xml_getElementAttr(node, name);

	if (attr) {
		escape_copy(attr->value, value, node->xml);
		node->xml->space += LEN(attr->value);
		return attr;
	}

	attr = acl_xml_attr_alloc(node);
	acl_vstring_strcpy(attr->name, name);
	node->xml->space += LEN(attr->name);
	escape_copy(attr->value, value, node->xml);
	node->xml->space += LEN(attr->value);
	acl_array_append(node->attr_list, attr);

	return attr;
}

ACL_XML_NODE *acl_xml_create_node(ACL_XML *xml, const char* tag,
	const char* text)
{
	ACL_XML_NODE *node = acl_xml_node_alloc(xml);

	acl_assert(tag && *tag);
	acl_vstring_strcpy(node->ltag, tag);
	xml->space += LEN(node->ltag);
	if (text && *text) {
		escape_copy(node->text, text, xml);
		xml->space += LEN(node->text);
	}

	return node;
}

ACL_XML_NODE *acl_xml_create_node_with_text_stream(ACL_XML *xml,
	const char *tag, ACL_VSTREAM *in, size_t off, size_t len)
{
	ACL_XML_NODE *node = acl_xml_node_alloc(xml);

	acl_assert(tag && *tag);
	acl_vstring_strcpy(node->ltag, tag);
	xml->space += LEN(node->ltag);
	if (in != NULL)
		acl_xml_node_set_text_stream(node, in, off, len);

	return node;
}

ACL_XML_ATTR *acl_xml_node_add_attr(ACL_XML_NODE *node, const char *name,
	const char *value)
{
	ACL_XML_ATTR *attr = acl_xml_attr_alloc(node);

	acl_assert(name && *name);
	acl_vstring_strcpy(attr->name, name);
	node->xml->space += LEN(attr->name);
	escape_copy(attr->value, value, node->xml);
	node->xml->space += LEN(attr->value);

	return attr;
}

void acl_xml_node_add_attrs(ACL_XML_NODE *node, ...)
{
	va_list ap;
	const char *name, *value;

	va_start(ap, node);
	while ((name = va_arg(ap, const char*)) != 0) {
		value = va_arg(ap, const char*);
		acl_assert(value);
		acl_xml_node_add_attr(node, name, value);
	}
	va_end(ap);
}

void acl_xml_node_set_text(ACL_XML_NODE *node, const char *text)
{
	size_t n1 = LEN(node->text), n2;

	if (text != NULL && *text != 0) {
		escape_copy(node->text, text, node->xml);
		n2 = LEN(node->text);
		if (n2 > n1)
			node->xml->space += n2 - n1;
	}
}

void acl_xml_node_add_text(ACL_XML_NODE *node, const char *text)
{
	size_t n1 = LEN(node->text), n2;

	if (text != NULL && *text != 0) {
		escape_append(node->text, text, node->xml);
		n2 = LEN(node->text);
		if (n2 > n1)
			node->xml->space += n2 - n1;
	}
}

void acl_xml_node_set_text_stream(ACL_XML_NODE *node, ACL_VSTREAM *in,
	size_t off, size_t len)
{
	const char *myname = "acl_xml_node_set_text_stream";
	int   ret;
	char  buf[8192];
	size_t n1 = LEN(node->text), n2, n;

	if (in == NULL)
		return;

	if (in->type == ACL_VSTREAM_TYPE_FILE
		&& acl_vstream_fseek(in, (acl_off_t) off, SEEK_SET) < 0)
	{
		const char *path = ACL_VSTREAM_PATH(in);

		acl_msg_error("%s(%d): fseek error: %s, file: %s, from: %lu",
			myname, __LINE__, acl_last_serror(),
			path ? path : "unknown", (unsigned long) off);
		return;
	}

	if (len == 0) {
		while (1) {
			ret = acl_vstream_read(in, buf, sizeof(buf) - 1);
			if (ret == ACL_VSTREAM_EOF)
				break;
			buf[ret] = 0;
			len -= ret;
			escape_copy(node->text, buf, node->xml);
		}

		n2 = LEN(node->text);
		if (n2 > n1)
			node->xml->space += n2 - n1;
		return;
	}

	while (len > 0) {
		n = len > sizeof(buf) - 1 ? sizeof(buf) - 1 : len;
		ret = acl_vstream_read(in, buf, n);
		if (ret == ACL_VSTREAM_EOF)
			break;
		buf[ret] = 0;
		len -= ret;
		escape_copy(node->text, buf, node->xml);
	}

	n2 = LEN(node->text);
	if (n2 > n1)
		node->xml->space += n2 - n1;
}

/***************************************************************************/

ACL_VSTRING *acl_xml_build(ACL_XML *xml, ACL_VSTRING *buf)
{
	ACL_XML_ATTR *attr;
	ACL_XML_NODE *node;
	ACL_ITER iter1, iter2;

	if (buf == NULL)
		buf = acl_vstring_alloc(256);

	acl_foreach(iter1, xml) {
		node = (ACL_XML_NODE*) iter1.data;

		if (ACL_XML_IS_CDATA(node)) {
			acl_vstring_strcat(buf, "<![CDATA[");
			if (LEN(node->text) > 0)
				acl_vstring_strcat(buf, STR(node->text));
		} else if (ACL_XML_IS_COMMENT(node)) {
			acl_vstring_strcat(buf, "<!--");
			acl_vstring_strcat(buf, STR(node->text));
		} else if ((node->flag & ACL_XML_F_META_QM)) {
			acl_vstring_strcat(buf, "<?");
			acl_vstring_strcat(buf, STR(node->ltag));
		} else if ((node->flag & ACL_XML_F_META_EM)) {
			acl_vstring_strcat(buf, "<!");
			acl_vstring_strcat(buf, STR(node->ltag));
			ACL_VSTRING_ADDCH(buf, ' ');
			if (LEN(node->text) > 0)
				acl_vstring_strcat(buf, STR(node->text));
		} else {
			ACL_VSTRING_ADDCH(buf, '<');
			acl_vstring_strcat(buf, STR(node->ltag));
		}

		acl_foreach(iter2, node->attr_list) {
			attr = (ACL_XML_ATTR*) iter2.data;
			ACL_VSTRING_ADDCH(buf, ' ');
			acl_vstring_strcat(buf, STR(attr->name));
			ACL_VSTRING_ADDCH(buf, '=');
			ACL_VSTRING_ADDCH(buf, '"');
			if (LEN(attr->value) > 0)
				acl_vstring_strcat(buf, STR(attr->value));
			ACL_VSTRING_ADDCH(buf, '"');
		}

		if (acl_ring_size(&node->children) > 0) {
			ACL_VSTRING_ADDCH(buf, '>');
			if (LEN(node->text) > 0)
				acl_vstring_strcat(buf, STR(node->text));
			continue;
		}
	       
		if (ACL_XML_IS_CDATA(node)) {
			acl_vstring_strcat(buf, "]]>");
		} else if (ACL_XML_IS_COMMENT(node)) {
			acl_vstring_strcat(buf, "-->");
		} else if (node->flag & ACL_XML_F_META_QM) {
			acl_vstring_strcat(buf, "?>");
		} else if (node->flag & ACL_XML_F_META_EM) {
			ACL_VSTRING_ADDCH(buf, '>');
		} else if (LEN(node->text) == 0) {
			acl_vstring_strcat(buf, "></");
			acl_vstring_strcat(buf, STR(node->ltag));
			ACL_VSTRING_ADDCH(buf, '>');
		} else {
			ACL_VSTRING_ADDCH(buf, '>');
			acl_vstring_strcat(buf, STR(node->text));
			acl_vstring_strcat(buf, "</");
			acl_vstring_strcat(buf, STR(node->ltag));
			ACL_VSTRING_ADDCH(buf, '>');
		}

		while (node->parent != node->xml->root
			&& acl_xml_node_next(node) == NULL)
		{
			acl_vstring_strcat(buf, "</");
			acl_vstring_strcat(buf, STR(node->parent->ltag));
			ACL_VSTRING_ADDCH(buf, '>');
			node = node->parent;
		}
	}

	ACL_VSTRING_TERMINATE(buf);
	return buf;
}

void acl_xml_dump(ACL_XML *xml, ACL_VSTREAM *fp)
{
	int   i;
	ACL_ITER iter1;
	const char *sep = "\t";

	acl_foreach(iter1, xml) {
		ACL_ITER iter2;
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter1.data;

		for (i = 1; i < node->depth; i++)
			acl_vstream_buffed_fprintf(fp, "%s", sep);

		if (ACL_XML_IS_COMMENT(node))
			acl_vstream_buffed_fprintf(fp, "comment> text: %s\n",
				STR(node->text));
		else
			acl_vstream_buffed_fprintf(fp, "tag> %s, text: %s\n",
				STR(node->ltag), STR(node->text));

		acl_foreach(iter2, node->attr_list) {
			ACL_XML_ATTR *attr = (ACL_XML_ATTR*) iter2.data;

			for (i = 1; i < node->depth; i++)
				acl_vstream_buffed_fprintf(fp, "%s", sep);

			acl_vstream_buffed_fprintf(fp, "%sattr> %s: %s\n",
				sep, STR(attr->name), STR(attr->value));
		}
	}
	acl_vstream_fflush(fp);
}

void acl_xml_dump2(ACL_XML *xml, ACL_VSTRING *buf)
{
	int   i;
	ACL_ITER iter1;
	const char *sep = "\t";

	acl_foreach(iter1, xml) {
		ACL_ITER iter2;
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter1.data;

		for (i = 1; i < node->depth; i++)
			acl_vstring_strcat(buf, sep);

		if (ACL_XML_IS_COMMENT(node))
			acl_vstring_sprintf_append(buf,
				"comment> text: %s\n", STR(node->text));
		else
			acl_vstring_sprintf_append(buf, "tag> %s, text: %s\n",
				STR(node->ltag), STR(node->text));

		acl_foreach(iter2, node->attr_list) {
			ACL_XML_ATTR *attr = (ACL_XML_ATTR*) iter2.data;

			for (i = 1; i < node->depth; i++)
				acl_vstring_strcat(buf, sep);

			acl_vstring_sprintf_append(buf, "%sattr> %s: %s\n",
				sep, STR(attr->name), STR(attr->value));
		}
	}
}
