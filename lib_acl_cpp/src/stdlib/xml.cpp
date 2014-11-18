#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/xml.hpp"

namespace acl {

xml_attr::xml_attr(void)
{
}

xml_attr::~xml_attr(void)
{
}

const char* xml_attr::get_name(void) const
{
	acl_assert(attr_);
	if (attr_->name)
		return acl_vstring_str(attr_->name);
	else
		return "";
}

const char* xml_attr::get_value(void) const
{
	acl_assert(attr_);
	if (attr_->value)
		return acl_vstring_str(attr_->value);
	else
		return "";
}

//////////////////////////////////////////////////////////////////////////

xml_node::xml_node(ACL_XML_NODE* node, xml* xml_ptr)
	: node_(node)
	, xml_(xml_ptr)
	, parent_(NULL)
	, parent_saved_(NULL)
	, child_(NULL)
	, child_iter_(NULL)
	, attr_(NULL)
	, attr_iter_(NULL)
{
	acl_assert(xml_ptr);
}

xml_node::~xml_node(void)
{
	delete parent_saved_;
	delete child_;
	if (child_iter_)
		acl_myfree(child_iter_);
	delete attr_;
	if (attr_iter_)
		acl_myfree(attr_iter_);
}

const char* xml_node::tag_name(void) const
{
	if (node_->ltag && ACL_VSTRING_LEN(node_->ltag) > 0)
		return (acl_vstring_str(node_->ltag));
	else
		return (NULL);
}

const char* xml_node::text(void) const
{
	if (node_->text && ACL_VSTRING_LEN(node_->text) > 0)
		return acl_vstring_str(node_->text);
	else
		return NULL;
}

const char* xml_node::id(void) const
{
	if (node_->id && ACL_VSTRING_LEN(node_->id) > 0)
		return acl_vstring_str(node_->id);
	else
		return NULL;
}

const char* xml_node::attr_value(const char* name) const
{
	return acl_xml_getElementAttrVal(node_, name);
}

const char* xml_node::operator[](const char* name) const
{
	return attr_value(name);
}

const xml_attr* xml_node::first_attr(void) const
{
	ACL_ARRAY* a = node_->attr_list;
	if (a == NULL)
		return NULL;

	if (attr_iter_ == NULL)
		const_cast<xml_node*>(this)->attr_iter_ =
			(ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));

	ACL_XML_ATTR* attr = (ACL_XML_ATTR*) a->iter_head(attr_iter_, a);
	if (attr == NULL)
		return NULL;
	if (attr_ == NULL)
		const_cast<xml_node*>(this)->attr_ = NEW xml_attr();
	const_cast<xml_node*>(this)->attr_->node_ = const_cast<xml_node*>(this);
	const_cast<xml_node*>(this)->attr_->attr_ = attr;
	return attr_;
}

const xml_attr* xml_node::next_attr(void) const
{
	ACL_ARRAY* a = node_->attr_list;
	if (a == NULL)
		return NULL;

	acl_assert(attr_iter_);
	acl_assert(attr_);

	ACL_XML_ATTR* attr = (ACL_XML_ATTR*) a->iter_next(attr_iter_, a);
	if (attr == NULL)
		return NULL;
	const_cast<xml_node*>(this)->attr_->node_ = const_cast<xml_node*>(this);
	const_cast<xml_node*>(this)->attr_->attr_ = attr;
	return attr_;
}

xml_node& xml_node::add_attr(const char* name, const char* value)
{
	acl_xml_node_add_attr(node_, name, value);
	return *this;
}

xml_node& xml_node::add_attr(const char* name, char n)
{
	char buf[2];
	snprintf(buf, sizeof(buf), "%c", n);
	return add_attr(name, buf);
}

xml_node& xml_node::add_attr(const char* name, int n)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%d", n);
	return add_attr(name, buf);
}

xml_node& xml_node::add_attr(const char* name, size_t n)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%lu", (unsigned long) n);
	return add_attr(name, buf);
}

xml_node& xml_node::add_attr(const char* name, acl_int64 n)
{
	char buf[32];
#ifdef WIN32
	snprintf(buf, sizeof(buf), "%I64d", n);
#else
	snprintf(buf, sizeof(buf), "%lld", n);
#endif
	return add_attr(name, buf);
}

xml_node& xml_node::set_text(const char* str)
{
	acl_xml_node_set_text(node_, str);
	return *this;
}

xml_node& xml_node::add_child(xml_node* child,
	bool return_child /* = false */)
{
	ACL_XML_NODE* node = child->get_xml_node();
	acl_xml_node_add_child(node_, node);
	child->parent_ = this;
	if (return_child)
		return *child;
	return *this;
}

xml_node& xml_node::add_child(xml_node& child,
	bool return_child /* = false */)
{
	return add_child(&child, return_child);
}

xml_node& xml_node::add_child(const char* tag,
	bool return_child /* = false */,
	const char* str /* = NULL */)
{
	return add_child(xml_->create_node(tag, str), return_child);
}

xml_node& xml_node::get_parent() const
{
	if (parent_)
		return *parent_;
	else if (node_->parent == node_->xml->root)
		return xml_->get_root();
	else if (node_->parent == NULL)  // xxx: can this happen?
		return xml_->get_root();

	const_cast<xml_node*>(this)->parent_saved_ =
		NEW xml_node(node_->parent, xml_);
	const_cast<xml_node*>(this)->parent_ = parent_saved_;
	return *parent_saved_;
}

xml_node* xml_node::first_child(void)
{
	if (child_iter_ == NULL)
		child_iter_ = (ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));

	ACL_XML_NODE* node = node_->iter_head(child_iter_, node_);
	if (node == NULL)
		return NULL;

	if (child_ == NULL)
		child_ = NEW xml_node(node, xml_);
	else
		child_->node_ = node;
	return child_;
}

xml_node* xml_node::next_child(void)
{
	acl_assert(child_iter_);
	acl_assert(child_);

	ACL_XML_NODE* node = node_->iter_next(child_iter_, node_);
	if (node == NULL)
		return NULL;
	child_->node_ = node;
	return child_;
}

int   xml_node::depth(void) const
{
	return node_->depth;
}

int   xml_node::children_count(void) const
{
	return acl_ring_size(&node_->children);
}

xml& xml_node::get_xml() const
{
	return *xml_;
}

ACL_XML_NODE* xml_node::get_xml_node() const
{
	return node_;
}

void xml_node::set_xml_node(ACL_XML_NODE* node)
{
	node_ = node;
}

//////////////////////////////////////////////////////////////////////

xml::xml(const char* data /* = NULL */)
{
	node_ = NULL;
	root_ = NULL;
	xml_ = acl_xml_alloc();
	buf_ = NULL;
	//dummyRootAdded_ = false;
	m_pTokenTree = NULL;
	iter_ = NULL;
	if (data && *data)
		update(data);
}

xml::~xml(void)
{
	clear();
	if (node_)
		delete node_;
	if (root_)
		delete root_;
	if (xml_)
		acl_xml_free(xml_);
	if (buf_)
		delete buf_;
	if (m_pTokenTree)
		acl_token_tree_destroy(m_pTokenTree);
	if (iter_)
		acl_myfree(iter_);
}

xml& xml::part_word(bool on)
{
	if (on)
		xml_->flag |= ACL_XML_FLAG_PART_WORD;
	else
		xml_->flag &= ~ACL_XML_FLAG_PART_WORD;
	return *this;
}

xml& xml::ignore_slash(bool on)
{
	if (on)
		xml_->flag |= ACL_XML_FLAG_IGNORE_SLASH;
	else
		xml_->flag &= ~ACL_XML_FLAG_IGNORE_SLASH;
	return *this;
}

void xml::update(const char* data)
{
	acl_xml_update(xml_, data);
}

const std::vector<xml_node*>& xml::getElementsByTagName(const char* tag) const
{
	const_cast<xml*>(this)->clear();

	ACL_ARRAY* a = acl_xml_getElementsByTagName(xml_, tag);
	if (a == NULL)
		return elements_;

	ACL_ITER iter;
	acl_foreach(iter, a)
	{
		ACL_XML_NODE *tmp = (ACL_XML_NODE*) iter.data;
		xml_node* node = NEW xml_node(tmp, const_cast<xml*>(this));
		const_cast<xml*>(this)->elements_.push_back(node);
	}
	acl_xml_free_array(a);

	return elements_;
}

const xml_node* xml::getFirstElementByTag(const char* tag) const
{
	ACL_ARRAY* a = acl_xml_getElementsByTagName(xml_, tag);
	if (a == NULL)
		return NULL;

	ACL_XML_NODE* node = (ACL_XML_NODE*) acl_array_index(a, 0);
	acl_assert(node);
	if (node_ == NULL)
		const_cast<xml*>(this)->node_ =
			NEW xml_node(node, const_cast<xml*>(this));
	else
		const_cast<xml*>(this)->node_->set_xml_node(node);
	acl_xml_free_array(a);

	return node_;
}

const std::vector<xml_node*>& xml::getElementsByTags(const char* tags) const
{
	const_cast<xml*>(this)->clear();

	ACL_ARRAY* a = acl_xml_getElementsByTags(xml_, tags);
	if (a == NULL)
		return elements_;

	ACL_ITER iter;
	acl_foreach(iter, a)
	{
		ACL_XML_NODE *tmp = (ACL_XML_NODE*) iter.data;
		xml_node* node = NEW xml_node(tmp, const_cast<xml*>(this));
		const_cast<xml*>(this)->elements_.push_back(node);
	}
	acl_xml_free_array(a);

	return elements_;
}

const xml_node* xml::getFirstElementByTags(const char* tags) const
{
	ACL_ARRAY* a = acl_xml_getElementsByTags(xml_, tags);
	if (a == NULL)
		return NULL;

	ACL_XML_NODE* node = (ACL_XML_NODE*) acl_array_index(a, 0);
	acl_assert(node);
	if (node_ == NULL)
		const_cast<xml*>(this)->node_ =
			NEW xml_node(node, const_cast<xml*>(this));
	else
		const_cast<xml*>(this)->node_->set_xml_node(node);
	acl_xml_free_array(a);

	return node_;
}

const std::vector<xml_node*>& xml::getElementsByName(const char* value) const
{
	const_cast<xml*>(this)->clear();
	ACL_ARRAY* a = acl_xml_getElementsByName(xml_, value);
	if (a == NULL)
		return elements_;

	ACL_ITER iter;
	acl_foreach(iter, a)
	{
		ACL_XML_NODE *tmp = (ACL_XML_NODE*) iter.data;
		xml_node* node = NEW xml_node(tmp, const_cast<xml*>(this));
		const_cast<xml*>(this)->elements_.push_back(node);
	}
	acl_xml_free_array(a);

	return elements_;
}

const std::vector<xml_node*>& xml::getElementsByAttr(
	const char *name, const char *value) const
{
	const_cast<xml*>(this)->clear();
	ACL_ARRAY *a = acl_xml_getElementsByAttr(xml_, name, value);
	if (a == NULL)
		return elements_;

	ACL_ITER iter;
	acl_foreach(iter, a)
	{
		ACL_XML_NODE *tmp = (ACL_XML_NODE*) iter.data;
		xml_node* node = NEW xml_node(tmp, const_cast<xml*>(this));
		const_cast<xml*>(this)->elements_.push_back(node);
	}
	acl_xml_free_array(a);

	return elements_;
}

const xml_node* xml::getElementById(const char* id) const
{
	ACL_XML_NODE* node = acl_xml_getElementById(xml_, id);
	if (node == NULL)
		return (NULL);

	if (node_ == NULL)
		const_cast<xml*>(this)->node_ =
			NEW xml_node(node, const_cast<xml*>(this));
	else
		const_cast<xml*>(this)->node_->set_xml_node(node);
	return node_;
}

ACL_XML* xml::get_xml(void) const
{
	return xml_;
}

const acl::string& xml::getText()
{
#define	STR(x)		acl_vstring_str((x))
#define	EQ(x, y)	!strcasecmp((x), (y))

	if (m_pTokenTree == NULL)
	{
		static const char* s = "&nbsp; &lt; &gt; &amp; &quot;"
			" &NBSP; &LT; &GT; &AMP; &QUOT;";
		m_pTokenTree = acl_token_tree_create2(s, " ");
	}

	string* pBuf = NEW string(1024);
	ACL_ITER iter;
	acl_foreach(iter, xml_)
	{
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter.data;

		if ((node->flag & ACL_XML_F_META))
			continue;
		if (EQ(STR(node->ltag), "style")
			|| EQ(STR(node->ltag), "meta")
			|| EQ(STR(node->ltag), "head")
			|| EQ(STR(node->ltag), "title")
			|| EQ(STR(node->ltag), "script"))
		{
			continue;
		}

		pBuf->append(STR(node->text));
	}

	if (buf_ == NULL)
		buf_ = NEW string(pBuf->length());
	else
		buf_->clear();

	const ACL_TOKEN *token;
	const char* ptr = pBuf->c_str(), *pLast;
	ACL_VSTRING* name = acl_vstring_alloc(8);
	while (*ptr)
	{
		pLast = ptr;
		ACL_TOKEN_TREE_MATCH(m_pTokenTree, ptr, NULL, NULL, token);
		if (token == NULL)
		{
			buf_->append(pLast, ptr - pLast);
			continue;
		}

		acl_token_name(token, name);

		if (EQ(STR(name), "&nbsp;"))
		{
			if (ptr - pLast - 6 > 0)
				buf_->append(pLast, ptr - pLast - 6);
			*buf_ += ' ';
		}
		else if (EQ(STR(name), "&amp;"))
		{
			if (ptr - pLast - 5 > 0)
				buf_->append(pLast, ptr - pLast - 5);
			*buf_ += '&';
		}
		else if (EQ(STR(name), "&lt;"))
		{
			if (ptr - pLast - 4 > 0)
				buf_->append(pLast, ptr - pLast - 4);
			*buf_ += '<';
		}
		else if (EQ(STR(name), "&gt;"))
		{
			if (ptr - pLast - 4 > 0)
				buf_->append(pLast, ptr - pLast - 4);
			*buf_ += '>';
		}
		else if (EQ(STR(name), "&quot;"))
		{
			if (ptr - pLast - 6 > 0)
				buf_->append(pLast, ptr - pLast - 6);
			*buf_ += '"';
		}
	}

	acl_vstring_free(name);
	delete pBuf;

	return *buf_;
}

xml_node& xml::create_node(const char* tag, const char* text /* = NULL */)
{
	ACL_XML_NODE* node = acl_xml_create_node(xml_, tag, text);
	xml_node* n = NEW xml_node(node, this);
	nodes_.push_back(n);
	return *n;
}

xml_node& xml::get_root(void)
{
	if (root_)
		return *root_;
	root_ = NEW xml_node(xml_->root, this);
	return *root_;
}

xml_node* xml::first_node(void)
{
	if (iter_ == NULL)
		iter_ = (ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));

	ACL_XML_NODE* node = xml_->iter_head(iter_, xml_);
	if (node == NULL)
		return NULL;

	if (node_ == NULL)
		node_ = NEW xml_node(node, this);
	else
		node_->set_xml_node(node);
	return node_;
}

xml_node* xml::next_node(void)
{
	acl_assert(iter_);
	acl_assert(node_);

	ACL_XML_NODE* node = xml_->iter_next(iter_, xml_);
	if (node == NULL)
		return NULL;
	node_->set_xml_node(node);
	return node_;
}

void xml::build_xml(string& out)
{
	ACL_VSTRING* buf = out.vstring();
	(void) acl_xml_build(xml_, buf);
}

void xml::reset(void)
{
	clear();
	if (xml_)
		acl_xml_reset(xml_);
	else
		xml_ = acl_xml_alloc();
	//dummyRootAdded_ = false;
}

int xml::push_pop(const char* in, size_t len acl_unused,
	string* out, size_t max /* = 0 */)
{
	//if (!dummyRootAdded_)
	//{
	//	update("<dummy_root>");
	//	dummyRootAdded_ = true;
	//}
	if (in)
		update(in);
	if (out == NULL)
		return (0);
	if (max > 0 && len > max)
		len = max;
	out->append(in, len);
	return len;
}

int xml::pop_end(string* out acl_unused, size_t max /* = 0 */ acl_unused)
{
	//if (dummyRootAdded_)
	//{
	//	update("</dummy_root>");
	//	dummyRootAdded_ = false;
	//}
	return 0;
}

void xml::clear(void)
{
	if (buf_)
		buf_->clear();

	std::vector<acl::xml_node*>::iterator it = elements_.begin();
	for (; it != elements_.end(); ++it)
		delete (*it);
	elements_.clear();

	std::list<xml_node*>::iterator it1 = nodes_.begin();
	for (; it1 != nodes_.end(); ++it1)
		delete (*it1);
}

} // namespace acl
