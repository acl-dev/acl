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
		return (1);
	else
		return (0);
}

int acl_xml_tag_leaf(const char *tag)
{
	if (strcasecmp(tag, "script") == 0)
		return (1);
	return (0);
}

void acl_xml_free_array(ACL_ARRAY *a)
{
	acl_array_destroy(a, NULL);
}

ACL_ARRAY *acl_xml_getElementsByTagName(ACL_XML *xml, const char *tag)
{
	ACL_ITER iter;
	ACL_ARRAY *a = acl_array_create(10);

	acl_foreach(iter, xml) {
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter.data;
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

ACL_ARRAY *acl_xml_getElementsByTags(ACL_XML *xml, const char *tags)
{
	ACL_ARGV *tokens = acl_argv_split(tags, "/");
	ACL_ARRAY *a, *ret;
	ACL_ITER iter;

	a = acl_xml_getElementsByTagName(xml, tokens->argv[tokens->argc - 1]);
	if (a == NULL) {
		acl_argv_free(tokens);
		return (NULL);
	}

	ret = acl_array_create(acl_array_size(a));

	acl_foreach(iter, a) {
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter.data, *parent = node;
		int   i = tokens->argc - 2;
		while (i >= 0 && (parent = parent->parent) != xml->root) {
			if (strcasecmp(tokens->argv[i], "*") != 0 &&
				strcasecmp(tokens->argv[i], STR(parent->ltag)) != 0)
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
	return (ret);
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
		return (NULL);
	}

	return (a);
}

ACL_ARRAY *acl_xml_getElementsByName(ACL_XML *xml, const char *value)
{
	return (acl_xml_getElementsByAttr(xml, "name", value));
}

ACL_XML_ATTR *acl_xml_getAttrById(ACL_XML *xml, const char *id)
{
	return (acl_htable_find(xml->id_table, id));
}

const char *acl_xml_getAttrValueById(ACL_XML *xml, const char *id)
{
	ACL_XML_ATTR *attr = acl_xml_getAttrById(xml, id);

	if (attr == NULL) {
		return (NULL);
	}
	return (STR(attr->value));
}

ACL_XML_NODE *acl_xml_getElementById(ACL_XML *xml, const char *id)
{
	ACL_XML_ATTR *attr = acl_xml_getAttrById(xml, id);

	if (attr == NULL)
		return (NULL);
	return (attr->node);
}
ACL_XML_NODE *acl_xml_getElementMeta(ACL_XML *xml, const char *tag)
{
	ACL_ITER iter;
	ACL_XML_NODE *node;

	acl_foreach(iter, xml) {
		node = (ACL_XML_NODE*) iter.data;
		if ((node->flag & ACL_XML_F_META_QM) == 0 || node->ltag == NULL)
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

		if (strcasecmp(STR(attr->name), name) == 0) {
			return (attr);
		}
	}

	return (NULL);
}

const char *acl_xml_getElementAttrVal(ACL_XML_NODE *node, const char *name)
{
	ACL_XML_ATTR *attr = acl_xml_getElementAttr(node, name);

	if (attr) {
		return (STR(attr->value));
	}

	return (NULL);
}

int acl_xml_removeElementAttr(ACL_XML_NODE *node, const char *name)
{
	ACL_XML_ATTR *attr = acl_xml_getElementAttr(node, name);

	if (attr == NULL) {
		return (-1);
	}

	if (acl_array_delete_obj(node->attr_list, attr, NULL) != 0) {
		return (-1);
	}

	if (node->id == attr->value) {
		acl_htable_delete(node->xml->id_table, STR(attr->value), NULL);
		node->id = NULL;
	}

	acl_xml_attr_free(attr);
	return (0);
}

ACL_XML_ATTR *acl_xml_addElementAttr(ACL_XML_NODE *node,
	const char *name, const char *value)
{
	ACL_XML_ATTR *attr = acl_xml_getElementAttr(node, name);

	if (attr) {
		acl_vstring_strcpy(attr->value, value);
		return (attr);
	}

	attr = acl_xml_attr_alloc(node);
	acl_vstring_strcpy(attr->name, name);
	acl_vstring_strcpy(attr->value, value);
	acl_array_append(node->attr_list, attr);

	return (attr);
}

ACL_XML_NODE *acl_xml_create_node(ACL_XML *xml, const char* tagname, const char* text)
{
	ACL_XML_NODE *node = acl_xml_node_alloc(xml);

	acl_assert(tagname && *tagname);
	acl_vstring_strcpy(node->ltag, tagname);
	if (text && *text)
		acl_vstring_strcpy(node->text, text);
	return (node);
}

ACL_XML_ATTR *acl_xml_node_add_attr(ACL_XML_NODE *node, const char *name, const char *value)
{
	ACL_XML_ATTR *attr = acl_xml_attr_alloc(node);

	acl_assert(name && *name);
	acl_vstring_strcpy(attr->name, name);
	if (value && *value)
		acl_vstring_strcpy(attr->value, value);
	return (attr);
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
	if (text && *text)
		acl_vstring_strcpy(node->text, text);
}

static void xml_escape_append(ACL_VSTRING *buf, const char *src, int quoted)
{
	const unsigned char *ptr = (const unsigned char*) src;

	if (quoted)
		ACL_VSTRING_ADDCH(buf, '"');
	while (*ptr) {
		if (*ptr == '"' || *ptr == '\\' || *ptr == '<' || *ptr == '>')
			ACL_VSTRING_ADDCH(buf, '\\');
		ACL_VSTRING_ADDCH(buf, *ptr);
		ptr++;
	}
	if (quoted)
		ACL_VSTRING_ADDCH(buf, '"');
	ACL_VSTRING_TERMINATE(buf);
}

ACL_VSTRING *acl_xml_build(ACL_XML *xml, ACL_VSTRING *buf)
{
	ACL_XML_ATTR *attr;
	ACL_XML_NODE *node;
	ACL_ITER iter1, iter2;

	if (buf == NULL)
		buf = acl_vstring_alloc(256);

	acl_foreach(iter1, xml) {
		node = (ACL_XML_NODE*) iter1.data;
		if (ACL_XML_IS_COMMENT(node)) {
			acl_vstring_strcat(buf, "<!-- ");
			xml_escape_append(buf, STR(node->text), 0);
		} else if ((node->flag & ACL_XML_F_META_QM)) {
			acl_vstring_sprintf_append(buf, "<?%s ", STR(node->ltag));
			if (LEN(node->text) > 0)
				xml_escape_append(buf, STR(node->text), 0);
		} else if ((node->flag & ACL_XML_F_META_EM)) {
			acl_vstring_sprintf_append(buf, "<!%s ", STR(node->ltag));
			if (LEN(node->text) > 0)
				xml_escape_append(buf, STR(node->text), 0);
		} else
			acl_vstring_sprintf_append(buf, "<%s", STR(node->ltag));

		acl_foreach(iter2, node->attr_list) {
			attr = (ACL_XML_ATTR*) iter2.data;
			acl_vstring_sprintf_append(buf, " %s=", STR(attr->name));
			xml_escape_append(buf, STR(attr->value), 1);
		}

		if (acl_ring_size(&node->children) > 0) {
			acl_vstring_strcat(buf, ">");
			if (LEN(node->text) > 0)
				acl_vstring_strcat(buf, STR(node->text));
			continue;
		}
		if (ACL_XML_IS_COMMENT(node)) {
			acl_vstring_strcat(buf, "-->");
			continue;
		}
		if ((node->flag & ACL_XML_F_META_QM) || (node->flag & ACL_XML_F_META_EM)) {
			acl_vstring_strcat(buf, ">");
			continue;
		}
		if (LEN(node->text) == 0)
			acl_vstring_strcat(buf, " />");
		else
			acl_vstring_sprintf_append(buf, ">%s</%s>",
				STR(node->text), STR(node->ltag));

		while (node->parent != node->xml->root && acl_xml_node_next(node) == NULL) {
			acl_vstring_sprintf_append(buf, "</%s>", STR(node->parent->ltag));
			node = node->parent;
		}
	}

	return (buf);
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
