#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <list>
#include "string.hpp"
#include "pipe_stream.hpp"
#include "xml.hpp"

struct ACL_XML;
struct ACL_XML_NODE;
struct ACL_XML_ATTR;

/**
 * Wrapper for XML parsing library in ACL library, convenient for C++ users. If performance is not a major concern,
 * can directly use this class. If executing on server side and performance is very important, it is recommended to directly use ACL library's
 * XML parser, because this class also calls XML parsing process in ACL library, and has a secondary copy
 * process, which may slightly affect performance, but for general applications this impact is negligible
 */

namespace acl {

class xml1;
class xml1_node;

class ACL_CPP_API xml1_attr : public xml_attr {
public:
	/**
	 * @override
	 */
	const char* get_name() const;

	/**
	 * @override
	 */
	const char* get_value() const;

protected:
	friend class xml1_node;

	xml1_attr(xml_node* node, ACL_XML_ATTR* attr);
	~xml1_attr() {}

private:
	ACL_XML_ATTR* attr_;
};

class ACL_CPP_API xml1_node : public xml_node
{
public:
	/**
	 * @override
	 */
	const char* tag_name() const;

	/**
	 * @override
	 */
	const char* id() const;

	/**
	 * @override
	 */
	const char* text() const;

	/**
	 * @override
	 */
	const char* attr_value(const char* name) const;

	/**
	 * @override
	 */
	const xml_attr* first_attr() const;

	/**
	 * @override
	 */
	const xml_attr* next_attr() const;

	/**
	 * @override
	 */
	xml_node& add_attr(const char* name, const char* value);

	/**
	 * @override
	 */
	xml_node& set_text(const char* str, bool append = false);

	/**
	 * @override
	 */
	xml_node& set_text(istream& in, size_t off = 0, size_t len = 0);

	/**
	 * @override
	 */
	xml_node& add_child(xml_node* child, bool return_child = false);

	/**
	 * @override
	 */
	int detach();

	/**
	 * @override
	 */
	xml_node& set_parent(xml_node* node);

	/**
	 * @override
	 */
	xml_node& get_parent() const;

	/**
	 * @override
	 */
	xml_node* first_child();

	/**
	 * @override
	 */
	xml_node* next_child();

	/**
	 * @override
	 */
	int depth() const;

	/**
	 * @override
	 */
	bool is_root() const;

	/**
	 * @override
	 */
	int children_count() const;

public:
	/**
	 * Get XML node object corresponding to ACL library
	 * @return {ACL_XML_NODE*} Returns node object. Note: Users cannot separately release this node
	 */
	ACL_XML_NODE* get_xml_node() const;

protected:
	friend class xml1;
	friend class dbuf_guard;

	/**
	 * XML node constructor
	 * @param xml_ptr {xml*} XML tree object, non-empty
	 * @param node {ACL_XML_NODE*} XML node pointer in C version
	 */
	xml1_node(xml* xml_ptr, ACL_XML_NODE* node);

	/**
	 * Requires that this object must be dynamically created
	 */
	~xml1_node();

private:
	ACL_XML_NODE *node_;
	ACL_ITER* child_iter_;
	ACL_ITER* attr_iter_;

	xml_node* parent_;
	xml1_node* parent_internal_;
};

class string;

class ACL_CPP_API xml1 : public xml {
public:
	/**
	 * Constructor
	 * @param data {const char*} XML format string, can be complete
	 *  XML string, can also be partial XML string, can also be NULL pointer.
	 *  In any case, users can still call update function with partial or complete XML string
	 *  to parse XML during update call; actually, when constructor's
	 *  data parameter is not empty, it will also call update
	 * @param dbuf_nblock {size_t} Initialization parameter for internally used dbuf_guard
	 * @param dbuf_capacity {size_t} Initialization parameter for internally used dbuf_guard
	 */
	xml1(const char* data = NULL, size_t dbuf_nblock = 2,
		size_t dbuf_capacity = 100);
	~xml1();

	/**
	 * @override
	 */
	xml& ignore_slash(bool on);

	/**
	 * @override
	 */
	xml& xml_decode(bool on);

	/**
	 * @override
	 */
	xml& xml_encode(bool on);

	/**
	 * @override
	 */
	xml& xml_multi_root(bool on);

	/**
	 * @override
	 */
	const char* update(const char* data);

	/**
	 * @override
	 */
	bool complete(const char* root_tag);

	/**
	 * @override
	 */
	void reset();

	/**
	 * @override
	 */
	const string& getText();

	/**
	 * @override
	 */
	const std::vector<xml_node*>&
		getElementsByTagName(const char* tag) const;

	/**
	 * @override
	 */
	xml_node* getFirstElementByTag(const char* tag) const;

	/**
	 * @override
	 */
	const std::vector<xml_node*>& getElementsByTags(const char* tags) const;

	/**
	 * @override
	 */
	xml_node* getFirstElementByTags(const char* tags) const;

	/**
	 * @override
	 */
	const std::vector<xml_node*>&
		getElementsByName(const char* value) const;

	/**
	 * @override
	 */
	const std::vector<xml_node*>& getElementsByAttr(
		const char* name, const char* value) const;

	/**
	 * @override
	 */
	xml_node* getElementById(const char* id) const;

	/**
	 * @override
	 */
	xml_node& create_node(const char* tag, const char* text = NULL);

	/**
	 * @override
	 */
	xml_node& create_node(const char* tag, istream& in,
		size_t off = 0, size_t len = 0);

	/**
	 * @override
	 */
	xml_node& get_root();

	/**
	 * @override
	 */
	xml_node* first_node();

	/**
	 * @override
	 */
	xml_node* next_node();

	/**
	 * @override
	 */
	void build_xml(string& out) const;

	/**
	 * @override
	 */
	const char* to_string(size_t* len = NULL) const;

	/**
	 * @override
	 */
	size_t space() const;

	/**
	 * @override
	 */
	void space_clear();

	/**
	 * @override
	 */
	size_t node_count() const;

	/**
	 * @override
	 */
	size_t attr_count() const;

public:
	/**
	 * Get ACL_XML object in acl library
	 * @return {ACL_XML*} This value cannot be NULL. Note: Users can modify the value of this object,
	 *  but cannot release this object
	 */
	ACL_XML* get_xml() const {
		return xml_;
	}

private:
	ACL_XML *xml_;
	ACL_ITER* iter_;
	xml1_node* root_;
};

} // namespace acl

