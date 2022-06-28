#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/xml.hpp"
#endif

namespace acl {

xml_node::xml_node(xml* xml_ptr)
	: xml_(xml_ptr)
{
	if (xml_ptr == NULL) {
		abort();
	}
}

xml_node::~xml_node(void)
{
	clear();
}

void xml_node::clear(void)
{
	std::vector<xml_node*>::iterator it = nodes_tmp_.begin();
	for (; it != nodes_tmp_.end(); ++it) {
		delete *it;
	}
	nodes_tmp_.clear();

	std::vector<xml_attr*>::iterator it2 = attrs_tmp_.begin();
	for (; it2 != attrs_tmp_.end(); ++it2) {
		delete *it2;
	}
	attrs_tmp_.clear();
}

xml& xml_node::get_xml(void) const
{
	return *xml_;
}

const char* xml_node::operator[](const char* name) const
{
	return attr_value(name);
}

xml_node& xml_node::add_attr(const char* name, char n)
{
	char buf[2];

	safe_snprintf(buf, sizeof(buf), "%c", n);
	return add_attr(name, buf);
}

xml_node& xml_node::add_attr(const char* name, int n)
{
	char buf[32];

	safe_snprintf(buf, sizeof(buf), "%d", n);
	return add_attr(name, buf);
}

xml_node& xml_node::add_attr(const char* name, size_t n)
{
	char buf[32];

	safe_snprintf(buf, sizeof(buf), "%lu", (unsigned long) n);
	return add_attr(name, buf);
}

xml_node& xml_node::add_attr(const char* name, acl_int64 n)
{
	char buf[32];

#ifdef ACL_WINDOWS
	safe_snprintf(buf, sizeof(buf), "%I64d", n);
#else
	safe_snprintf(buf, sizeof(buf), "%lld", n);
#endif
	return add_attr(name, buf);
}

xml_node& xml_node::set_text(acl_int64 number)
{
	char buf[32];

	if (acl_i64toa(number, buf, sizeof(buf)) == NULL) {
		abort();
	}
	return set_text(buf);
}

xml_node& xml_node::add_child(xml_node& child, bool return_child /* = false */)
{
	return add_child(&child, return_child);
}

xml_node& xml_node::add_child(const char* tag, bool return_child /* = false */,
	const char* str /* = NULL */)
{
	return add_child(xml_->create_node(tag, str), return_child);
}

xml_node& xml_node::add_child(const char* tag, const char* txt,
	bool return_child /* = false */)
{
	return add_child(xml_->create_node(tag, txt), return_child);
}

xml_node& xml_node::add_child(const char* tag, acl_int64 number,
	bool return_child /* = false */)
{
	return add_child(xml_->create_node(tag, number), return_child);
}

xml_node& xml_node::add_child(const char* tag, istream& in, size_t off /* = 0 */,
	size_t len /* = 0 */, bool return_child /* = false */)
{
	return add_child(xml_->create_node(tag, in, off, len), return_child);
}

//////////////////////////////////////////////////////////////////////

xml::xml(size_t dbuf_nblock /* = 2 */, size_t dbuf_capacity /* = 100 */)
: dbuf_(dbuf_nblock, dbuf_capacity)
{
	//dummyRootAdded_ = false;
	buf_ = NULL;
	m_pTokenTree = NULL;
}

xml::~xml(void)
{
	clear();

	delete buf_;
	if (m_pTokenTree) {
		acl_token_tree_destroy(m_pTokenTree);
	}
}

void xml::clear(void)
{
	if (buf_) {
		buf_->clear();
	}

	//std::vector<acl::xml_node*>::iterator it = elements_.begin();
	//for (; it != elements_.end(); ++it)
	//	delete (*it);
	elements_.clear();

	//std::list<xml_node*>::iterator it1 = nodes_tmp_.begin();
	//for (; it1 != nodes_tmp_.end(); ++it1)
	//	delete (*it1);
	//nodes_tmp_.clear();

	dbuf_.dbuf_reset();
}

const acl::string& xml::getText(void)
{
	if (buf_ == NULL)
		buf_ = NEW string();
	else
		buf_->clear();
	return *buf_;
}

xml_node& xml::create_node(const char* tag, acl_int64 number)
{
	char buf[32];
	if (acl_i64toa(number, buf, sizeof(buf)) == NULL) {
		abort();
	}

	return create_node(tag, buf);
}

int xml::push_pop(const char* in, size_t len acl_unused,
	string* out, size_t max /* = 0 */)
{
	//if (!dummyRootAdded_)
	//{
	//	update("<dummy_root>");
	//	dummyRootAdded_ = true;
	//}
	if (in) {
		update(in);
	}
	if (out == NULL) {
		return 0;
	}
	if (max > 0 && len > max) {
		len = max;
	}
	out->append(in, len);
	return (int) len;
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

} // namespace acl
