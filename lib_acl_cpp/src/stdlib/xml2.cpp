#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/fstream.hpp"
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/stdlib/xml2.hpp"
#endif

namespace acl {

xml2_attr::xml2_attr(xml_node* node, ACL_XML2_ATTR* attr)
: xml_attr(node)
, attr_(attr)
{
	acl_assert(attr_);
}

const char* xml2_attr::get_name(void) const
{
	return attr_->name;
}

const char* xml2_attr::get_value(void) const
{
	return attr_->value;
}

//////////////////////////////////////////////////////////////////////////

xml2_node::xml2_node(xml* xml_ptr, ACL_XML2_NODE* node)
: xml_node(xml_ptr)
, node_(node)
, child_iter_(NULL)
, attr_iter_(NULL)
, parent_(NULL)
, parent_internal_(NULL)
{
	acl_assert(node);
}

xml2_node::~xml2_node(void)
{
	delete parent_internal_;
	if (child_iter_) {
		acl_myfree(child_iter_);
	}
	if (attr_iter_) {
		acl_myfree(attr_iter_);
	}
}

ACL_XML2_NODE* xml2_node::get_xml_node(void) const
{
	return node_;
}

const char* xml2_node::tag_name(void) const
{
	return node_->ltag;
}

const char* xml2_node::text(void) const
{
	return node_->text;
}

const char* xml2_node::id(void) const
{
	return node_->id;
}

const char* xml2_node::attr_value(const char* name) const
{
	return acl_xml2_getElementAttrVal(node_, name);
}

const xml_attr* xml2_node::first_attr(void) const
{
	ACL_ARRAY* a = node_->attr_list;
	if (a == NULL) {
		return NULL;
	}

	if (attr_iter_ == NULL) {
		const_cast<xml2_node*>(this)->attr_iter_ =
			(ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));
	}

	ACL_XML2_ATTR* attr = (ACL_XML2_ATTR*) a->iter_head(attr_iter_, a);
	if (attr == NULL) {
		return NULL;
	}

	xml2_attr* xa = NEW xml2_attr(const_cast<xml2_node*>(this), attr);
	const_cast<xml2_node*>(this)->attrs_tmp_.push_back(xa);
	return xa;
}

const xml_attr* xml2_node::next_attr(void) const
{
	ACL_ARRAY* a = node_->attr_list;
	if (a == NULL) {
		return NULL;
	}

	acl_assert(attr_iter_);

	ACL_XML2_ATTR* attr = (ACL_XML2_ATTR*) a->iter_next(attr_iter_, a);
	if (attr == NULL) {
		return NULL;
	}

	xml2_attr* xa = NEW xml2_attr(const_cast<xml2_node*>(this), attr);
	const_cast<xml2_node*>(this)->attrs_tmp_.push_back(xa);
	return xa;

}

xml_node& xml2_node::add_attr(const char* name, const char* value)
{
	acl_xml2_node_add_attr(node_, name, value);
	return *this;
}

xml_node& xml2_node::set_text(const char* str, bool append /* = false */)
{
	if (append) {
		acl_xml2_node_add_text(node_, str);
	} else {
		acl_xml2_node_set_text(node_, str);
	}
	return *this;
}

xml_node& xml2_node::set_text(istream& in, size_t off /* = 0 */,
	size_t len /* = 0 */)
{
	acl_xml2_node_set_text_stream(node_, in.get_vstream(), off, len);
	return *this;
}

xml_node& xml2_node::add_child(xml_node* child, bool return_child /* = false */)
{
	ACL_XML2_NODE* node = ((xml2_node*) child)->get_xml_node();
	acl_xml2_node_add_child(node_, node);
	child->set_parent(this);

	if (return_child) {
		return *child;
	}
	return *this;
}

int xml2_node::detach(void)
{
	return acl_xml2_node_delete(node_);
}

xml_node& xml2_node::set_parent(xml_node* parent)
{
	parent_ = parent;
	return *this;
}

xml_node& xml2_node::get_parent(void) const
{
	if (parent_) {
		return *parent_;
	} else if (node_->parent == node_->xml->root) {
		return xml_->get_root();
	} else if (node_->parent == NULL) { // xxx
		return xml_->get_root();
	}

	xml2_node* node = NEW xml2_node(xml_, node_->parent);
	const_cast<xml2_node*>(this)->parent_internal_ = node;
	const_cast<xml2_node*>(this)->parent_ = node;

	return *node;
}

xml_node* xml2_node::first_child(void)
{
	if (child_iter_ == NULL) {
		child_iter_ = (ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));
	}

	ACL_XML2_NODE* node = node_->iter_head(child_iter_, node_);
	if (node == NULL) {
		return NULL;
	}

	xml2_node* n = NEW xml2_node(xml_, node);
	nodes_tmp_.push_back(n);

	return n;
}

xml_node* xml2_node::next_child(void)
{
	acl_assert(child_iter_);

	ACL_XML2_NODE* node = node_->iter_next(child_iter_, node_);
	if (node == NULL) {
		return NULL;
	}

	xml2_node* n = NEW xml2_node(xml_, node);
	nodes_tmp_.push_back(n);

	return n;
}

int xml2_node::depth(void) const
{
	return node_->depth;
}

bool xml2_node::is_root(void) const
{
	xml2_node& node = (xml2_node&) ((xml2*) xml_)->get_root();
	ACL_XML2_NODE* root = node.get_xml_node();
	return root == node_;
}

int xml2_node::children_count(void) const
{
	return acl_ring_size(&node_->children);
}

//////////////////////////////////////////////////////////////////////

xml2::xml2(const char* filepath, size_t max_len, const char* data /* = NULL */,
	size_t init_len /* = 8192 */, size_t dbuf_nblock /* = 2 */,
	size_t dbuf_capacity /* = 100 */)
: xml(dbuf_nblock, dbuf_capacity)
{
	acl_assert(filepath && max_len > 0 && init_len > 0);

	if (max_len < init_len) {
		max_len = init_len;
	}

	iter_ = NULL;
	root_ = NULL;

	xml_ = acl_xml2_mmap_file(filepath, max_len, init_len, NULL);

	if (data && *data) {
		update(data);
	}
}

xml2::xml2(fstream& fp, size_t max_len, const char* data /* = NULL */,
	size_t init_len /* = 8192 */, size_t dbuf_nblock /* = 2 */,
	size_t dbuf_capacity /* = 100 */)
: xml(dbuf_nblock, dbuf_capacity)
{
	acl_assert(max_len > 0 && init_len > 0);

	if (max_len < init_len) {
		max_len = init_len;
	}

	iter_ = NULL;
	root_ = NULL;

	xml_ = acl_xml2_mmap_fd(fp.file_handle(),
		max_len, init_len, NULL);

	if (data && *data) {
		update(data);
	}
}

xml2::xml2(ACL_FILE_HANDLE fd, size_t max_len, const char* data /* = NULL */,
	size_t init_len /* = 8192 */, size_t dbuf_nblock /* = 2 */,
	size_t dbuf_capacity /* = 100 */)
: xml(dbuf_nblock, dbuf_capacity)
{
	acl_assert(fd != ACL_FILE_INVALID);
	acl_assert(max_len > 0);

	if (init_len > max_len) {
		max_len = init_len;
	}

	xml_ = acl_xml2_mmap_fd(fd, max_len, init_len, NULL);

	if (data && *data) {
		update(data);
	}
}

xml2::~xml2(void)
{
	if (iter_) {
		acl_myfree(iter_);
	}
	delete root_;
	acl_xml2_free(xml_);
}

xml& xml2::ignore_slash(bool on)
{
	acl_xml2_slash(xml_, on ? 1 : 0);
	return *this;
}

xml& xml2::xml_decode(bool on)
{
	acl_xml2_decode_enable(xml_, on ? 1 : 0);
	return *this;
}

xml& xml2::xml_encode(bool on)
{
	acl_xml2_encode_enable(xml_, on ? 1 : 0);
	return *this;
}

xml& xml2::xml_multi_root(bool on)
{
	acl_xml2_multi_root(xml_, on ? 1 : 0);
	return *this;
}

const char* xml2::update(const char* data)
{
	return acl_xml2_update(xml_, data);
}

bool xml2::complete(const char* root_tag)
{
	return acl_xml2_is_complete(xml_, root_tag) != 0 ? true : false;
}

const std::vector<xml_node*>& xml2::getElementsByTagName(const char* tag) const
{
	const_cast<xml2*>(this)->clear();

	ACL_ARRAY* a = acl_xml2_getElementsByTagName(xml_, tag);
	if (a == NULL) {
		return elements_;
	}

	ACL_ITER iter;
	acl_foreach(iter, a) {
		ACL_XML2_NODE *tmp = (ACL_XML2_NODE*) iter.data;
		xml2_node* node = const_cast<dbuf_guard&>(dbuf_)
			.create<xml2_node, xml2*, ACL_XML2_NODE*>
			(const_cast<xml2*>(this), tmp);
		const_cast<xml2*>(this)->elements_.push_back(node);
	}

	acl_xml_free_array(a);
	return elements_;
}

xml_node* xml2::getFirstElementByTag(const char* tag) const
{
	ACL_XML2_NODE* node = acl_xml2_getFirstElementByTagName(xml_, tag);
	if (node == NULL) {
		return NULL;
	}

	xml2_node* n = const_cast<dbuf_guard&>(dbuf_)
		.create<xml2_node, xml2*, ACL_XML2_NODE*>
		(const_cast<xml2*>(this), node);
	return n;
}

const std::vector<xml_node*>& xml2::getElementsByTags(const char* tags) const
{
	const_cast<xml2*>(this)->clear();

	ACL_ARRAY* a = acl_xml2_getElementsByTags(xml_, tags);
	if (a == NULL) {
		return elements_;
	}

	ACL_ITER iter;
	acl_foreach(iter, a) {
		ACL_XML2_NODE *tmp = (ACL_XML2_NODE*) iter.data;
		xml2_node* node = const_cast<dbuf_guard&>(dbuf_)
			.create<xml2_node, xml2*, ACL_XML2_NODE*>
			(const_cast<xml2*>(this), tmp);
		const_cast<xml2*>(this)->elements_.push_back(node);
	}

	acl_xml_free_array(a);
	return elements_;
}

xml_node* xml2::getFirstElementByTags(const char* tags) const
{
	ACL_ARRAY* a = acl_xml2_getElementsByTags(xml_, tags);
	if (a == NULL) {
		return NULL;
	}

	ACL_XML2_NODE* node = (ACL_XML2_NODE*) acl_array_index(a, 0);
	acl_assert(node);

	xml2_node* n = const_cast<dbuf_guard&>(dbuf_)
		.create<xml2_node, xml2*, ACL_XML2_NODE*>
		(const_cast<xml2*>(this), node);

	acl_xml_free_array(a);
	return n;
}

const std::vector<xml_node*>& xml2::getElementsByName(const char* value) const
{
	const_cast<xml2*>(this)->clear();
	ACL_ARRAY* a = acl_xml2_getElementsByName(xml_, value);
	if (a == NULL) {
		return elements_;
	}

	ACL_ITER iter;
	acl_foreach(iter, a) {
		ACL_XML2_NODE *tmp = (ACL_XML2_NODE*) iter.data;
		xml2_node* node = const_cast<dbuf_guard&>(dbuf_)
			.create<xml2_node, xml2*, ACL_XML2_NODE*>
			(const_cast<xml2*>(this), tmp);
		const_cast<xml2*>(this)->elements_.push_back(node);
	}

	acl_xml_free_array(a);
	return elements_;
}

const std::vector<xml_node*>& xml2::getElementsByAttr(
	const char *name, const char *value) const
{
	const_cast<xml2*>(this)->clear();
	ACL_ARRAY *a = acl_xml2_getElementsByAttr(xml_, name, value);
	if (a == NULL) {
		return elements_;
	}

	ACL_ITER iter;
	acl_foreach(iter, a) {
		ACL_XML2_NODE *tmp = (ACL_XML2_NODE*) iter.data;
		xml2_node* node = const_cast<dbuf_guard&>(dbuf_)
			.create<xml2_node, xml2*, ACL_XML2_NODE*>
			(const_cast<xml2*>(this), tmp);
		const_cast<xml2*>(this)->elements_.push_back(node);
	}

	acl_xml_free_array(a);
	return elements_;
}

xml_node* xml2::getElementById(const char* id) const
{
	ACL_XML2_NODE* node = acl_xml2_getElementById(xml_, id);
	if (node == NULL) {
		return NULL;
	}

	xml2_node* n = const_cast<dbuf_guard&>(dbuf_)
		.create<xml2_node, xml2*, ACL_XML2_NODE*>
		(const_cast<xml2*>(this), node);
	return n;
}

const acl::string& xml2::getText(void)
{
	logger_error("not supported yet!");

	if (buf_ == NULL) {
		buf_ = NEW string();
	} else {
		buf_->clear();
	}
	return *buf_;
}

xml_node& xml2::create_node(const char* tag, const char* text /* = NULL */)
{
	ACL_XML2_NODE* node = acl_xml2_create_node(xml_, tag, text);
	xml2_node* n = dbuf_.create<xml2_node, xml2*, ACL_XML2_NODE*>
		(this, node);
	return *n;
}

xml_node& xml2::create_node(const char* tag, istream& in,
	size_t off /* = 0 */, size_t len /* = 0 */)
{
	ACL_XML2_NODE* node = acl_xml2_create_node_with_text_stream(xml_, tag,
		in.get_vstream(), off, len);
	xml2_node* n = dbuf_.create<xml2_node, xml2*, ACL_XML2_NODE*>
		(this, node);
	return *n;
}

xml_node& xml2::get_root(void)
{
	if (root_) {
		return *root_;
	}
	root_ = NEW xml2_node(this, xml_->root);
	return *root_;
}

xml_node* xml2::first_node(void)
{
	if (iter_ == NULL) {
		iter_ = (ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));
	}

	ACL_XML2_NODE* node = xml_->iter_head(iter_, xml_);
	if (node == NULL) {
		return NULL;
	}

	xml2_node* n = dbuf_.create<xml2_node, xml2*, ACL_XML2_NODE*>
		(this, node);
	return n;
}

xml_node* xml2::next_node(void)
{
	acl_assert(iter_);

	ACL_XML2_NODE* node = xml_->iter_next(iter_, xml_);
	if (node == NULL) {
		return NULL;
	}

	xml2_node* n = dbuf_.create<xml2_node, xml2*, ACL_XML2_NODE*>
		(this, node);
	return n;
}

void xml2::build_xml(string& out) const
{
	(void) acl_xml2_build2(xml_, out.vstring());
}

const char* xml2::to_string(size_t* len /* = NULL */) const
{
	const char* dat = acl_xml2_build(xml_);
	if (dat >= acl_vstring_end(xml_->vbuf)) {
		if (len) {
			*len = 0;
		}
		return "";
	}

	if (len) {
		*len = acl_vstring_end(xml_->vbuf) - dat;
	}
	return dat;
}

void xml2::reset(void)
{
	clear();
	delete root_;
	root_ = NULL;
	acl_xml2_reset(xml_);
	//dummyRootAdded_ = false;
}

size_t xml2::space(void) const
{
	return acl_xml2_space(xml_);
}

void xml2::space_clear(void)
{
	acl_xml2_space_clear(xml_);
}

size_t xml2::node_count(void) const
{
	return (size_t) xml_->node_cnt;
}

size_t xml2::attr_count(void) const
{
	return (size_t) xml_->attr_cnt;
}

} // namespace acl
