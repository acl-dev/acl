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

class ACL_CPP_API xml2_attr : public xml_attr
{
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
	~xml2_attr(void) {}

private:
	ACL_XML2_ATTR* attr_;
};

class ACL_CPP_API xml2_node : public xml_node
{
public:
	/**
	 * @override
	 */
	const char* tag_name(void) const;

	/**
	 * @override
	 */
	const char* id(void) const;

	/**
	 * @override
	 */
	const char* text(void) const;

	/**
	 * @override
	 */
	const char* attr_value(const char* name) const;

	/**
	 * @override
	 */
	const xml_attr* first_attr(void) const;

	/**
	 * @override
	 */
	const xml_attr* next_attr(void) const;

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
	int detach(void);

	/**
	 * @override
	 */
	xml_node& set_parent(xml_node* node);

	/**
	 * @override
	 */
	xml_node& get_parent(void) const;

	/**
	 * @override
	 */
	xml_node* first_child(void);

	/**
	 * @override
	 */
	xml_node* next_child(void);

	/**
	 * @override
	 */
	int depth(void) const;

	/**
	 * @override
	 */
	bool is_root(void) const;

	/**
	 * @override
	 */
	int children_count(void) const;

public:
	void set_xml2_node(ACL_XML2_NODE* node);

	ACL_XML2_NODE* get_xml_node(void) const;

protected:
	friend class xml2;
	friend class dbuf_guard;

	xml2_node(xml* xml_ptr, ACL_XML2_NODE* node);
	~xml2_node(void);

private:
	ACL_XML2_NODE *node_;
	ACL_ITER* child_iter_;
	ACL_ITER* attr_iter_;

	xml_node* parent_;
	xml2_node* parent_internal_;
};

class fstream;

class ACL_CPP_API xml2 : public xml
{
public:
	/**
	 * 构造函数，使 xml 对象树创建在指定内存映射文件上
	 * @param filepath {const char*} 内存映射文件名
	 * @param max_len {size_t} 内存映射文件的最大大小，即内存映射文件在自动
	 *  增长时不应超过此大小
	 * @param data {const char*} 非空时自动调用解析过程
	 * @param init_len {size_t} 内存映射文件创建时的初始大小
	 * @param dbuf_nblock {size_t} 内部所用 dbuf_guard 的初始化参数
	 * @param dbuf_capacity {size_t} 内部所用 dbuf_guard 的初始化参数
	 */
	xml2(const char* filepath, size_t max_len, const char* data = NULL,
		size_t init_len = 8192, size_t dbuf_nblock = 2,
		size_t dbuf_capacity = 100);

	/**
	 * 构造函数，使 xml 对象树创建在指定内存映射文件上
	 * @param fp {fstream&} 内存映射文件句柄，该文件句柄应在本 xml 对象
	 *  释放前一直保持打开状态，即 fp 必须在 xml 对象释放后才可以关闭
	 * @param max_len {size_t} 内存映射文件的最大大小，即内存映射文件在自动
	 *  增长时不应超过此大小
	 * @param data {const char*} 非空时自动调用解析过程
	 * @param init_len {size_t} 内存映射文件创建时的初始大小
	 * @param dbuf_nblock {size_t} 内部所用 dbuf_guard 的初始化参数
	 * @param dbuf_capacity {size_t} 内部所用 dbuf_guard 的初始化参数
	 */
	xml2(fstream& fp, size_t max_len, const char* data = NULL,
		size_t init_len = 8192, size_t dbuf_nblock = 2,
		size_t dbuf_capacity = 100);

	/**
	 * 构造函数，使 xml 对象树创建在指定内存映射文件上
	 * @param fd {ACL_FILE_HANDLE} 内存映射文件句柄，该文件句柄应在本 xml
	 *  对象释放前一直保持打开状态，即 fp 必须在 xml 对象释放后才可以关闭
	 * @param max_len {size_t} 内存映射文件的最大大小，即内存映射文件在自动
	 *  增长时不应超过此大小
	 * @param data {const char*} 非空时自动调用解析过程
	 * @param init_len {size_t} 内存映射文件创建时的初始大小
	 * @param dbuf_nblock {size_t} 内部所用 dbuf_guard 的初始化参数
	 * @param dbuf_capacity {size_t} 内部所用 dbuf_guard 的初始化参数
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

	~xml2(void);

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
	void reset(void);

	/**
	 * @override
	 */
	const string& getText(void);

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
	xml_node& get_root(void);

	/**
	 * @override
	 */
	xml_node* first_node(void);

	/**
	 * @override
	 */
	xml_node* next_node(void);

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
	size_t space(void) const;

	/**
	 * @override
	 */
	void space_clear(void);

	/**
	 * @override
	 */
	size_t node_count(void) const;

	/**
	 * @override
	 */
	size_t attr_count(void) const;

public:
	ACL_XML2* get_xml2(void) const
	{
		return xml_;
	}

private:
	ACL_XML2* xml_;
	ACL_ITER* iter_;
	xml2_node* root_;
};

} // namespace acl
