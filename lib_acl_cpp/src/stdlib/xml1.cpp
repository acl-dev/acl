#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/stdlib/xml1.hpp"
#endif

namespace acl {

xml1_attr::xml1_attr(xml_node* node, ACL_XML_ATTR* attr)
: xml_attr(node)
, attr_(attr)
{
	acl_assert(attr_);
}

const char* xml1_attr::get_name(void) const
{
	if (attr_->name) {
		return acl_vstring_str(attr_->name);
	} else {
		return "";
	}
}

const char* xml1_attr::get_value(void) const
{
	if (attr_->value) {
		return acl_vstring_str(attr_->value);
	} else {
		return "";
	}
}

//////////////////////////////////////////////////////////////////////////

xml1_node::xml1_node(xml* xml_ptr, ACL_XML_NODE* node)
: xml_node(xml_ptr)
, node_(node)
, child_iter_(NULL)
, attr_iter_(NULL)
, parent_(NULL)
, parent_internal_(NULL)
{
}

xml1_node::~xml1_node(void)
{
	delete parent_internal_;
	if (child_iter_) {
		acl_myfree(child_iter_);
	}
	if (attr_iter_) {
		acl_myfree(attr_iter_);
	}
}

ACL_XML_NODE* xml1_node::get_xml_node(void) const
{
	return node_;
}

const char* xml1_node::tag_name(void) const
{
	if (node_->ltag && ACL_VSTRING_LEN(node_->ltag) > 0) {
		return (acl_vstring_str(node_->ltag));
	} else {
		return "";
	}
}

const char* xml1_node::text(void) const
{
	if (node_->text && ACL_VSTRING_LEN(node_->text) > 0) {
		return acl_vstring_str(node_->text);
	} else {
		return "";
	}
}

const char* xml1_node::id(void) const
{
	if (node_->id && ACL_VSTRING_LEN(node_->id) > 0) {
		return acl_vstring_str(node_->id);
	} else {
		return "";
	}
}

const char* xml1_node::attr_value(const char* name) const
{
	return acl_xml_getElementAttrVal(node_, name);
}

const xml_attr* xml1_node::first_attr(void) const
{
	ACL_ARRAY* a = node_->attr_list;
	if (a == NULL) {
		return NULL;
	}

	if (attr_iter_ == NULL) {
		const_cast<xml1_node*>(this)->attr_iter_ =
			(ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));
	}

	ACL_XML_ATTR* attr = (ACL_XML_ATTR*) a->iter_head(attr_iter_, a);
	if (attr == NULL) {
		return NULL;
	}

	xml1_attr* xa = NEW xml1_attr(const_cast<xml1_node*>(this), attr);
	const_cast<xml1_node*>(this)->attrs_tmp_.push_back(xa);
	return xa;
}

const xml_attr* xml1_node::next_attr(void) const
{
	ACL_ARRAY* a = node_->attr_list;
	if (a == NULL) {
		return NULL;
	}

	acl_assert(attr_iter_);

	ACL_XML_ATTR* attr = (ACL_XML_ATTR*) a->iter_next(attr_iter_, a);
	if (attr == NULL) {
		return NULL;
	}

	xml1_attr* xa = NEW xml1_attr(const_cast<xml1_node*>(this), attr);
	const_cast<xml1_node*>(this)->attrs_tmp_.push_back(xa);
	return xa;
}

xml_node& xml1_node::add_attr(const char* name, const char* value)
{
	acl_xml_node_add_attr(node_, name, value);
	return *this;
}

xml_node& xml1_node::set_text(const char* str, bool append /* = false */)
{
	if (append) {
		acl_xml_node_add_text(node_, str);
	} else {
		acl_xml_node_set_text(node_, str);
	}
	return *this;
}

xml_node& xml1_node::set_text(istream& in, size_t off /* = 0 */,
	size_t len /* = 0 */)
{
	acl_xml_node_set_text_stream(node_, in.get_vstream(),  off, len);
	return *this;
}

xml_node& xml1_node::add_child(xml_node* child, bool return_child /* = false */)
{
	ACL_XML_NODE* node = ((xml1_node*) child)->get_xml_node();
	acl_xml_node_add_child(node_, node);
	child->set_parent(this);

	if (return_child) {
		return *child;
	}
	return *this;
}

int xml1_node::detach(void)
{
	return acl_xml_node_delete(node_);
}

xml_node& xml1_node::set_parent(xml_node* parent)
{
	parent_ = parent;
	return *this;
}

xml_node& xml1_node::get_parent() const
{
	if (parent_) {
		return *parent_;
	} else if (node_->parent == node_->xml->root) {
		return xml_->get_root();
	} else if (node_->parent == NULL) { // xxx: can this happen?
		return xml_->get_root();
	}

	xml1_node* node = NEW xml1_node(xml_, node_->parent);
	const_cast<xml1_node*>(this)->parent_internal_ = node;
	const_cast<xml1_node*>(this)->parent_ = node;

	return *node;
}

xml_node* xml1_node::first_child(void)
{
	if (child_iter_ == NULL) {
		child_iter_ = (ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));
	}

	ACL_XML_NODE* node = node_->iter_head(child_iter_, node_);
	if (node == NULL) {
		return NULL;
	}

	xml1_node* n = NEW xml1_node(xml_, node);
	nodes_tmp_.push_back(n);
	return n;
}

xml_node* xml1_node::next_child(void)
{
	acl_assert(child_iter_);

	ACL_XML_NODE* node = node_->iter_next(child_iter_, node_);
	if (node == NULL) {
		return NULL;
	}

	xml1_node* n = NEW xml1_node(xml_, node);
	nodes_tmp_.push_back(n);

	return n;
}

int xml1_node::depth(void) const
{
	return node_->depth;
}

bool xml1_node::is_root(void) const
{
	xml1_node& node = (xml1_node&) ((xml1*) xml_)->get_root();
	ACL_XML_NODE* root = node.get_xml_node();
	return root == node_;
}

int xml1_node::children_count(void) const
{
	return acl_ring_size(&node_->children);
}

//////////////////////////////////////////////////////////////////////

xml1::xml1(const char* data /* = NULL */, size_t dbuf_nblock /* = 2 */,
	size_t dbuf_capacity /* = 100 */)
: xml(dbuf_nblock, dbuf_capacity)
{
	iter_ = NULL;
	root_ = NULL;

	xml_ = acl_xml_alloc();
	if (data && *data) {
		update(data);
	}
}

xml1::~xml1(void)
{
	if (iter_)
		acl_myfree(iter_);
	delete root_;
	acl_xml_free(xml_);
}

xml& xml1::ignore_slash(bool on)
{
	acl_xml_slash(xml_, on ? 1 : 0);
	return *this;
}

xml& xml1::xml_decode(bool on)
{
	acl_xml_decode_enable(xml_, on ? 1 : 0);
	return *this;
}

xml& xml1::xml_encode(bool on)
{
	acl_xml_encode_enable(xml_, on ? 1 : 0);
	return *this;
}

xml& xml1::xml_multi_root(bool on)
{
	acl_xml_multi_root(xml_, on ? 1 : 0);
	return *this;
}

const char* xml1::update(const char* data)
{
	return acl_xml_update(xml_, data);
}

bool xml1::complete(const char* root_tag)
{
	return acl_xml_is_complete(xml_, root_tag) != 0 ? true : false;
}

const std::vector<xml_node*>& xml1::getElementsByTagName(const char* tag) const
{
	const_cast<xml1*>(this)->clear();

	ACL_ARRAY* a = acl_xml_getElementsByTagName(xml_, tag);
	if (a == NULL) {
		return elements_;
	}

	ACL_ITER iter;
	acl_foreach(iter, a) {
		ACL_XML_NODE *tmp = (ACL_XML_NODE*) iter.data;
		xml1_node* node = const_cast<dbuf_guard&>(dbuf_)
			.create<xml1_node, xml1*, ACL_XML_NODE*>
			(const_cast<xml1*>(this), tmp);
		const_cast<xml1*>(this)->elements_.push_back(node);
	}

	acl_xml_free_array(a);
	return elements_;
}

xml_node* xml1::getFirstElementByTag(const char* tag) const
{
	ACL_XML_NODE* node = acl_xml_getFirstElementByTagName(xml_, tag);
	if (node == NULL) {
		return NULL;
	}

	xml1_node* n = const_cast<dbuf_guard&>(dbuf_)
		.create<xml1_node, xml1*, ACL_XML_NODE*>
		(const_cast<xml1*>(this), node);
	return n;
}

const std::vector<xml_node*>& xml1::getElementsByTags(const char* tags) const
{
	const_cast<xml1*>(this)->clear();

	ACL_ARRAY* a = acl_xml_getElementsByTags(xml_, tags);
	if (a == NULL) {
		return elements_;
	}

	ACL_ITER iter;
	acl_foreach(iter, a) {
		ACL_XML_NODE *tmp = (ACL_XML_NODE*) iter.data;
		xml1_node* node = const_cast<dbuf_guard&>(dbuf_)
			.create<xml1_node, xml1*, ACL_XML_NODE*>
			(const_cast<xml1*>(this), tmp);
		const_cast<xml1*>(this)->elements_.push_back(node);
	}

	acl_xml_free_array(a);
	return elements_;
}

xml_node* xml1::getFirstElementByTags(const char* tags) const
{
	ACL_ARRAY* a = acl_xml_getElementsByTags(xml_, tags);
	if (a == NULL) {
		return NULL;
	}

	ACL_XML_NODE* node = (ACL_XML_NODE*) acl_array_index(a, 0);
	acl_assert(node);

	xml1_node* n = const_cast<dbuf_guard&>(dbuf_)
		.create<xml1_node, xml1*, ACL_XML_NODE*>
		(const_cast<xml1*>(this), node);

	acl_xml_free_array(a);
	return n;
}

const std::vector<xml_node*>& xml1::getElementsByName(const char* value) const
{
	const_cast<xml1*>(this)->clear();
	ACL_ARRAY* a = acl_xml_getElementsByName(xml_, value);
	if (a == NULL) {
		return elements_;
	}

	ACL_ITER iter;
	acl_foreach(iter, a) {
		ACL_XML_NODE *tmp = (ACL_XML_NODE*) iter.data;
		xml1_node* node = const_cast<dbuf_guard&>(dbuf_)
			.create<xml1_node, xml1*, ACL_XML_NODE*>
			(const_cast<xml1*>(this), tmp);
		const_cast<xml1*>(this)->elements_.push_back(node);
	}

	acl_xml_free_array(a);
	return elements_;
}

const std::vector<xml_node*>& xml1::getElementsByAttr(
	const char *name, const char *value) const
{
	const_cast<xml1*>(this)->clear();
	ACL_ARRAY *a = acl_xml_getElementsByAttr(xml_, name, value);
	if (a == NULL) {
		return elements_;
	}

	ACL_ITER iter;
	acl_foreach(iter, a) {
		ACL_XML_NODE *tmp = (ACL_XML_NODE*) iter.data;
		xml1_node* node = const_cast<dbuf_guard&>(dbuf_)
			.create<xml1_node, xml1*, ACL_XML_NODE*>
			(const_cast<xml1*>(this), tmp);
		const_cast<xml1*>(this)->elements_.push_back(node);
	}

	acl_xml_free_array(a);
	return elements_;
}

xml_node* xml1::getElementById(const char* id) const
{
	ACL_XML_NODE* node = acl_xml_getElementById(xml_, id);
	if (node == NULL) {
		return NULL;
	}

	xml1_node* n = const_cast<dbuf_guard&>(dbuf_)
		.create<xml1_node, xml1*, ACL_XML_NODE*>
		(const_cast<xml1*>(this), node);
	return n;
}

const acl::string& xml1::getText(void)
{
#define	STR(x)		acl_vstring_str((x))
#define	EQ(x, y)	!strcasecmp((x), (y))

	if (m_pTokenTree == NULL) {
		static const char* s = "&nbsp; &lt; &gt; &amp; &quot;"
			" &NBSP; &LT; &GT; &AMP; &QUOT;";
		m_pTokenTree = acl_token_tree_create2(s, " ");
	}

	string* pBuf = NEW string(1024);
	ACL_ITER iter;
	acl_foreach(iter, xml_) {
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter.data;

		if ((node->flag & ACL_XML_F_META)) {
			continue;
		}

		if (EQ(STR(node->ltag), "style")
			|| EQ(STR(node->ltag), "meta")
			|| EQ(STR(node->ltag), "head")
			|| EQ(STR(node->ltag), "title")
			|| EQ(STR(node->ltag), "script")) {

			continue;
		}

		pBuf->append(STR(node->text));
	}

	if (buf_ == NULL) {
		buf_ = NEW string(pBuf->length());
	} else {
		buf_->clear();
	}

	ACL_TOKEN *token;
	const char* ptr   = pBuf->c_str(), *pLast;
	ACL_VSTRING* name = acl_vstring_alloc(8);

	while (*ptr) {
		pLast = ptr;
		ACL_TOKEN_TREE_MATCH(m_pTokenTree, ptr, NULL, NULL, token);
		if (token == NULL) {
			buf_->append(pLast, ptr - pLast);
			continue;
		}

		acl_token_name(token, name);

		if (EQ(STR(name), "&nbsp;")) {
			if (ptr - pLast - 6 > 0) {
				buf_->append(pLast, ptr - pLast - 6);
			}
			*buf_ += ' ';
		} else if (EQ(STR(name), "&amp;")) {
			if (ptr - pLast - 5 > 0) {
				buf_->append(pLast, ptr - pLast - 5);
			}
			*buf_ += '&';
		} else if (EQ(STR(name), "&lt;")) {
			if (ptr - pLast - 4 > 0) {
				buf_->append(pLast, ptr - pLast - 4);
			}
			*buf_ += '<';
		} else if (EQ(STR(name), "&gt;")) {
			if (ptr - pLast - 4 > 0) {
				buf_->append(pLast, ptr - pLast - 4);
			}
			*buf_ += '>';
		} else if (EQ(STR(name), "&quot;")) {
			if (ptr - pLast - 6 > 0) {
				buf_->append(pLast, ptr - pLast - 6);
			}
			*buf_ += '"';
		}
	}

	acl_vstring_free(name);
	delete pBuf;

	return *buf_;
}

xml_node& xml1::create_node(const char* tag, const char* text /* = NULL */)
{
	ACL_XML_NODE* node = acl_xml_create_node(xml_, tag, text);
	xml1_node* n = dbuf_.create<xml1_node, xml1*, ACL_XML_NODE*>
		(this, node);
	return *n;
}

xml_node& xml1::create_node(const char* tag, istream& in,
	size_t off /* = 0 */, size_t len /* = 0 */)
{
	ACL_XML_NODE* node = acl_xml_create_node_with_text_stream(xml_, tag,
		in.get_vstream(), off, len);
	xml1_node* n = dbuf_.create<xml1_node, xml1*, ACL_XML_NODE*>
		(this, node);
	return *n;
}

xml_node& xml1::get_root(void)
{
	if (root_) {
		return *root_;
	}
	root_ = NEW xml1_node(this, xml_->root);
	return *root_;
}

xml_node* xml1::first_node(void)
{
	if (iter_ == NULL) {
		iter_ = (ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));
	}

	ACL_XML_NODE* node = xml_->iter_head(iter_, xml_);
	if (node == NULL) {
		return NULL;
	}

	xml1_node* n = dbuf_.create<xml1_node, xml1*, ACL_XML_NODE*>
		(this, node);
	return n;
}

xml_node* xml1::next_node(void)
{
	acl_assert(iter_);

	ACL_XML_NODE* node = xml_->iter_next(iter_, xml_);
	if (node == NULL) {
		return NULL;
	}

	xml1_node* n = dbuf_.create<xml1_node, xml1*, ACL_XML_NODE*>
		(this, node);
	return n;
}

void xml1::build_xml(string& out) const
{
	(void) acl_xml_build(xml_, out.vstring());
}

const char* xml1::to_string(size_t* len /* = NULL */) const
{
	if (buf_ == NULL) {
		const_cast<xml1*>(this)->buf_ = NEW string();
	} else {
		const_cast<xml1*>(this)->buf_->clear();
	}

	build_xml(*buf_);
	if (len) {
		*len = buf_->size();
	}
	return buf_->c_str();
}

void xml1::reset(void)
{
	clear();
	delete root_;
	root_ = NULL;
	acl_xml_reset(xml_);
	//dummyRootAdded_ = false;
}

size_t xml1::space(void) const
{
	return acl_xml_space(xml_);
}

void xml1::space_clear(void)
{
	acl_xml_space_clear(xml_);
}

size_t xml1::node_count(void) const
{
	return (size_t) xml_->node_cnt;
}

size_t xml1::attr_count(void) const
{
	return (size_t) xml_->attr_cnt;
}

} // namespace acl
