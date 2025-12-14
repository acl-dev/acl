#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <list>
#include "string.hpp"
#include "xml.hpp"

struct ACL_XML2;
struct ACL_XML2_NODE;
struct ACL_XML2_ATTR;

namespace acl {

class xml2;
class xml2_node;

class ACL_CPP_API xml2_attr : public xml_attr {
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
	friend class xml2_node;

	xml2_attr(xml_node* node, ACL_XML2_ATTR* attr);
	~xml2_attr() {}

private:
	ACL_XML2_ATTR* attr_;
};

class ACL_CPP_API xml2_node : public xml_node
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
	void set_xml2_node(ACL_XML2_NODE* node);

	ACL_XML2_NODE* get_xml_node() const;

protected:
	friend class xml2;
	friend class dbuf_guard;

	xml2_node(xml* xml_ptr, ACL_XML2_NODE* node);
	~xml2_node();

private:
	ACL_XML2_NODE *node_;
	ACL_ITER* child_iter_;
	ACL_ITER* attr_iter_;

	xml_node* parent_;
	xml2_node* parent_internal_;
};

class fstream;

class ACL_CPP_API xml2 : public xml {
public:
	/**
	 * Constructor, create XML object tree on specified memory-mapped file
	 * @param filepath {const char*} Memory-mapped filename
	 * @param max_len {size_t} Maximum size of memory-mapped file, i.e., memory-mapped file should not exceed
	 *  this size when automatically growing
	 * @param data {const char*} When not empty, automatically calls parsing process
	 * @param init_len {size_t} Initial size when memory-mapped file is created
	 * @param dbuf_nblock {size_t} Initialization parameter for internally used dbuf_guard
	 * @param dbuf_capacity {size_t} Initialization parameter for internally used dbuf_guard
	 */
	xml2(const char* filepath, size_t max_len, const char* data = NULL,
		size_t init_len = 8192, size_t dbuf_nblock = 2,
		size_t dbuf_capacity = 100);

	/**
	 * Constructor, create XML object tree on specified memory-mapped file
	 * @param fp {fstream&} Memory-mapped file handle. This file handle should remain open
	 *  until this xml object is released, i.e., fp must be closed only after xml object is released
	 * @param max_len {size_t} Maximum size of memory-mapped file, i.e., memory-mapped file should not exceed
	 *  this size when automatically growing
	 * @param data {const char*} When not empty, automatically calls parsing process
	 * @param init_len {size_t} Initial size when memory-mapped file is created
	 * @param dbuf_nblock {size_t} Initialization parameter for internally used dbuf_guard
	 * @param dbuf_capacity {size_t} Initialization parameter for internally used dbuf_guard
	 */
	xml2(fstream& fp, size_t max_len, const char* data = NULL,
		size_t init_len = 8192, size_t dbuf_nblock = 2,
		size_t dbuf_capacity = 100);

	/**
	 * Constructor, create XML object tree on specified memory-mapped file
	 * @param fd {ACL_FILE_HANDLE} Memory-mapped file handle. This file handle should remain open
	 *  until this xml object is released, i.e., fp must be closed only after xml object is released
	 * @param max_len {size_t} Maximum size of memory-mapped file, i.e., memory-mapped file should not exceed
	 *  this size when automatically growing
	 * @param data {const char*} When not empty, automatically calls parsing process
	 * @param init_len {size_t} Initial size when memory-mapped file is created
	 * @param dbuf_nblock {size_t} Initialization parameter for internally used dbuf_guard
	 * @param dbuf_capacity {size_t} Initialization parameter for internally used dbuf_guard
	 */
#if defined(_WIN32) || defined(_WIN64)
	xml2(void* fd,  size_t max_len, const char* data = NULL,
		size_t init_len = 8192, size_t dbuf_nblock = 2,
		size_t dbuf_capacity = 100);
#else
	xml2(int fd, size_t max_len, const char* data = NULL,
		size_t init_len = 8192, size_t dbuf_nblock = 2,
		size_t dbuf_capacity = 100);
#endif

	~xml2();

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
	const std::vector<xml_node*>&
		getElementsByTags(const char* tags) const;

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
	ACL_XML2* get_xml2() const {
		return xml_;
	}

private:
	ACL_XML2* xml_;
	ACL_ITER* iter_;
	xml2_node* root_;
};

} // namespace acl

