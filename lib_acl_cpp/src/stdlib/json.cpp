#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/json.hpp"

namespace acl
{

json_node::json_node(ACL_JSON_NODE* node, json* json_ptr)
: node_me_(node)
, json_(json_ptr)
, parent_(NULL)
, parent_saved_(NULL)
, child_(NULL)
, iter_(NULL)
, buf_(NULL)
, obj_(NULL)
{
	acl_assert(json_ptr);
}

json_node::~json_node(void)
{
	delete parent_saved_;
	delete child_;
	if (iter_)
		acl_myfree(iter_);
	delete buf_;
	delete obj_;
}

const char* json_node::tag_name(void) const
{
	if (node_me_->ltag && ACL_VSTRING_LEN(node_me_->ltag) > 0)
		return (acl_vstring_str(node_me_->ltag));
	else
		return (NULL);
}

const char* json_node::get_text(void) const
{
	if (node_me_->text && ACL_VSTRING_LEN(node_me_->text) > 0)
		return acl_vstring_str(node_me_->text);
	else
		return NULL;
}

json_node* json_node::get_obj(void) const
{
	if (obj_ != NULL)
		return obj_;
	if (node_me_->tag_node == NULL)
		return NULL;

	const_cast<json_node*>(this)->obj_ =
		NEW json_node(node_me_->tag_node, json_);
	return obj_;
}

bool json_node::set_tag(const char* name)
{
	if (name == NULL || *name == 0)
		return false;
	if (node_me_->ltag == NULL || ACL_VSTRING_LEN(node_me_->ltag) == 0)
		return false;
	acl_vstring_strcpy(node_me_->ltag, name);
	return true;
}

bool json_node::set_text(const char* text)
{
	if (text == NULL || *text == 0)
		return false;
	if (node_me_->type != ACL_JSON_T_LEAF || node_me_->text == NULL)
		return false;
	acl_vstring_strcpy(node_me_->text, text);
	return true;
}

const string& json_node::to_string(void)
{
	if (buf_ == NULL)
		buf_ = NEW string(256);
	else
		buf_->clear();
	ACL_VSTRING* vbuf = buf_->vstring();
	(void) acl_json_node_build(node_me_, vbuf);
	return *buf_;
}

json_node& json_node::add_child(json_node* child,
	bool return_child /* = false */)
{
	ACL_JSON_NODE* node = child->get_json_node();
	// 先添加 child 至父结点中
	acl_json_node_add_child(node_me_, node);
	child->parent_ = this;
	if (return_child)
		return *child;
	return *this;
}

json_node& json_node::add_child(json_node& child,
	bool return_child /* = false */)
{
	return add_child(&child, return_child);
}

json_node& json_node::add_array(bool return_child /* = false */)
{
	return add_child(json_->create_array(), return_child);
}

json_node& json_node::add_child(bool as_array /* = false */,
	bool return_child /* = false */)
{
	return add_child(json_->create_node(as_array), return_child);
}

json_node& json_node::add_text(const char* tag, const char* value,
	bool return_child /* = false */)
{
	return add_child(json_->create_node(tag, value), return_child);
}

json_node& json_node::add_child(const char* tag, const char* value,
	bool return_child /* = false */)
{
	return add_text(tag, value, return_child);
}

json_node& json_node::add_number(const char* tag, acl_int64 value,
	bool return_child /* = false */)
{
	return add_child(json_->create_node(tag, value), return_child);
}

json_node& json_node::add_bool(const char* tag, bool value,
	bool return_child /* = false */)
{
	return add_child(json_->create_node(tag, value), return_child);
}

json_node& json_node::add_array_text(const char* text, bool return_child /* = false */)
{
	return add_child(json_->create_array_text(text), return_child);
}

json_node& json_node::add_child(const char* text, bool return_child /* = false */)
{
	return add_array_text(text, return_child);
}

json_node& json_node::add_array_number(acl_int64 value, bool return_child /* = false */)
{
	return add_child(json_->create_array_number(value), return_child);
}

json_node& json_node::add_array_bool(bool value, bool return_child /* = false */)
{
	return add_child(json_->create_array_bool(value), return_child);
}

json_node& json_node::add_child(const char* tag, json_node* node,
	bool return_child /* = false */)
{
	return add_child(json_->create_node(tag, node), return_child);
}

json_node& json_node::add_child(const char* tag, json_node& node,
	bool return_child /* = false */)
{
	return add_child(tag, &node, return_child);
}

json_node& json_node::get_parent() const
{
	if (parent_)
		return *parent_;
	else if (node_me_->parent == node_me_->json->root)
		return json_->get_root();
	else if (node_me_->parent == NULL)  // xxx: can this happen?
		return json_->get_root();

	const_cast<json_node*>(this)->parent_saved_ =
		NEW json_node(node_me_->parent, json_);
	const_cast<json_node*>(this)->parent_ = parent_saved_;
	return *parent_saved_;
}

json_node* json_node::first_child(void)
{
	if (iter_ == NULL)
		iter_ = (ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));

	ACL_JSON_NODE* node = node_me_->iter_head(iter_, node_me_);
	if (node == NULL)
		return NULL;

	if (child_ == NULL)
		child_ = NEW json_node(node, json_);
	else
		child_->node_me_ = node;
	return child_;
}

json_node* json_node::next_child(void)
{
	acl_assert(iter_);
	acl_assert(child_);

	ACL_JSON_NODE* node = node_me_->iter_next(iter_, node_me_);
	if (node == NULL)
		return NULL;
	child_->node_me_ = node;
	return child_;
}

int   json_node::depth(void) const
{
	return node_me_->depth;
}

int   json_node::children_count(void) const
{
	return acl_ring_size(&node_me_->children);
}

json& json_node::get_json(void) const
{
	return *json_;
}

ACL_JSON_NODE* json_node::get_json_node() const
{
	return node_me_;
}

void json_node::set_json_node(ACL_JSON_NODE* node)
{
	node_me_ = node;
}

//////////////////////////////////////////////////////////////////////////

json::json(const char* data /* = NULL */)
{
	json_ = acl_json_alloc();
	root_ = NULL;
	node_tmp_ = NULL;
	buf_ = NULL;
	iter_ = NULL;
	if (data && *data)
		update(data);
}

json::json(const json_node& node)
{
	json_ = acl_json_create(node.get_json_node());
	root_ = NEW json_node(json_->root, this);
	node_tmp_ = NULL;
	buf_ = NULL;
	iter_ = NULL;
}

json::~json(void)
{
	clear();
	if (json_)
		acl_json_free(json_);
	if (root_)
		delete root_;
	if (node_tmp_)
		delete node_tmp_;
	if (buf_)
		delete buf_;
	if (iter_)
		acl_myfree(iter_);
}

json& json::part_word(bool on)
{
	if (on)
		json_->flag |= ACL_JSON_FLAG_PART_WORD;
	else
		json_->flag &= ~ACL_JSON_FLAG_PART_WORD;
	return *this;
}

void json::update(const char* data)
{
	acl_json_update(json_, data);
}

const std::vector<json_node*>& json::getElementsByTagName(const char* tag) const
{
	const_cast<json*>(this)->clear();
	ACL_ARRAY* a = acl_json_getElementsByTagName(json_, tag);
	if (a == NULL)
		return nodes_tmp_;

	ACL_ITER iter;
	acl_foreach(iter, a)
	{
		ACL_JSON_NODE *tmp = (ACL_JSON_NODE*) iter.data;
		json_node* node = NEW json_node(tmp, const_cast<json*>(this));
		const_cast<json*>(this)->nodes_tmp_.push_back(node);
	}
	acl_json_free_array(a);

	return nodes_tmp_;
}

const std::vector<json_node*>& json::getElementsByTags(const char* tags) const
{
	const_cast<json*>(this)->clear();

	ACL_ARRAY* a = acl_json_getElementsByTags(json_, tags);
	if (a == NULL)
		return nodes_tmp_;

	ACL_ITER iter;
	acl_foreach(iter, a)
	{
		ACL_JSON_NODE *tmp = (ACL_JSON_NODE*) iter.data;
		json_node* node = NEW json_node(tmp, const_cast<json*>(this));
		const_cast<json*>(this)->nodes_tmp_.push_back(node);
	}
	acl_json_free_array(a);

	return nodes_tmp_;
}

ACL_JSON* json::get_json(void) const
{
	return json_;
}

json_node& json::create_node(const char* tag, const char* value)
{
	ACL_JSON_NODE* node = acl_json_create_text(json_, tag, value);
	json_node* n = NEW json_node(node, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::create_node(const char* tag, acl_int64 value)
{
	ACL_JSON_NODE* node = acl_json_create_int64(json_, tag, value);
	json_node* n = NEW json_node(node, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::create_node(const char* tag, bool value)
{
	ACL_JSON_NODE* node = acl_json_create_bool(json_, tag, value ? 1 : 0);
	json_node* n = NEW json_node(node, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::create_array_text(const char* text)
{
	ACL_JSON_NODE* node = acl_json_create_array_text(json_, text);
	json_node* n = NEW json_node(node, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::create_node(const char* text)
{
	return create_array_text(text);
}

json_node& json::create_array_number(acl_int64 value)
{
	ACL_JSON_NODE* node = acl_json_create_array_int64(json_, value);
	json_node* n = NEW json_node(node, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::create_array_bool(bool value)
{
	ACL_JSON_NODE* node = acl_json_create_array_bool(json_, value);
	json_node* n = NEW json_node(node, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::create_node(bool as_array /* = false */)
{
	if (as_array)
		return create_array();

	ACL_JSON_NODE* node = acl_json_create_obj(json_);
	json_node* n = NEW json_node(node, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::create_array()
{
	ACL_JSON_NODE* node = acl_json_create_array(json_);
	json_node* n = NEW json_node(node, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::create_node(const char* tag, json_node* node)
{
	ACL_JSON_NODE* tmp = acl_json_create_node(json_,
		tag, node->get_json_node());
	json_node* n = NEW json_node(tmp, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::create_node(const char* tag, json_node& node)
{
	ACL_JSON_NODE* tmp = acl_json_create_node(json_,
		tag, node.get_json_node());
	json_node* n = NEW json_node(tmp, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::duplicate_node(const json_node* node)
{
	ACL_JSON_NODE* tmp = acl_json_node_duplicate(json_,
		node->get_json_node());
	acl_assert(tmp);
	json_node* n = NEW json_node(tmp, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::duplicate_node(const json_node& node)
{
	ACL_JSON_NODE* tmp = acl_json_node_duplicate(json_,
		node.get_json_node());
	acl_assert(tmp);
	json_node* n = NEW json_node(tmp, this);
	nodes_.push_back(n);
	return *n;
}

json_node& json::get_root(void)
{
	if (root_)
		return *root_;
	root_ = NEW json_node(json_->root, this);
	return *root_;
}

json_node* json::first_node(void)
{
	if (iter_ == NULL)
		iter_ = (ACL_ITER*) acl_mymalloc(sizeof(ACL_ITER));
	ACL_JSON_NODE* node = json_->iter_head(iter_, json_);
	if (node == NULL)
		return NULL;
	if (node_tmp_ == NULL)
		node_tmp_ = NEW json_node(node, this);
	else
		node_tmp_->set_json_node(node);
	return node_tmp_;
}

json_node* json::next_node(void)
{
	acl_assert(iter_);
	acl_assert(node_tmp_);
	ACL_JSON_NODE* node = json_->iter_next(iter_, json_);
	if (node == NULL)
		return NULL;
	node_tmp_->set_json_node(node);
	return node_tmp_;
}

const string& json::to_string(void)
{
	if (buf_ == NULL)
		buf_ = new string(256);
	else
		buf_->clear();
	ACL_VSTRING* vbuf = buf_->vstring();
	(void) acl_json_build(json_, vbuf);
	return *buf_;
}

void json::build_json(string& out)
{
	ACL_VSTRING* buf = out.vstring();
	(void) acl_json_build(json_, buf);
}

void json::reset(void)
{
	clear();
	if (json_)
		acl_json_reset(json_);
	else
		json_ = acl_json_alloc();
}

int json::push_pop(const char* in, size_t len acl_unused,
	string* out, size_t max /* = 0 */)
{
	if (in)
		update(in);
	if (out == NULL)
		return (0);
	if (max > 0 && len > max)
		len = max;
	out->append(in, len);
	return len;
}

int json::pop_end(string* out acl_unused, size_t max /* = 0 */ acl_unused)
{
	return 0;
}

void json::clear(void)
{
	if (buf_)
		buf_->clear();

	std::vector<json_node*>::iterator it = nodes_tmp_.begin();
	for (; it != nodes_tmp_.end(); ++it)
		delete (*it);
	nodes_tmp_.clear();

	std::list<json_node*>::iterator it1 = nodes_.begin();
	for (; it1 != nodes_.end(); ++it1)
		delete (*it1);
}

} // namespace acl
