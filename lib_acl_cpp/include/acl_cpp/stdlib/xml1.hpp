#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include <list>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/pipe_stream.hpp"
#include "acl_cpp/stdlib/xml.hpp"

struct ACL_XML;
struct ACL_XML_NODE;
struct ACL_XML_ATTR;

/**
 * 对 ACL 库中 XML 解析库的封装，方便 C++ 用户使用，如果不太注重性能因素，
 * 可以直接使用该类，如果在服务端执行且非常注重性能，建议直接使用 ACL 库的
 * XML 解析器，因为该类也是调用了 ACL 库中的 XML 解析过程，并且有二次拷贝
 * 过程，可能会稍微影响一些性能，但对于一般的应用这点影响是微不足道的
 */

namespace acl {

class xml1;
class xml1_node;

class ACL_CPP_API xml1_attr : public xml_attr
{
public:
	/**
	 * @override
	 */
	const char* get_name(void) const;

	/**
	 * @override
	 */
	const char* get_value(void) const;

protected:
	friend class xml1_node;

	xml1_attr(xml_node* node, ACL_XML_ATTR* attr);
	~xml1_attr(void) {}

private:
	ACL_XML_ATTR* attr_;
};

class ACL_CPP_API xml1_node : public xml_node
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
	/**
	 * 取出对应于 ACL 库中的 XML 节点对象
	 * @return {ACL_XML_NODE*} 返回节点对象，注：该节点用户不能单独释放
	 */
	ACL_XML_NODE* get_xml_node(void) const;

protected:
	friend class xml1;
	friend class dbuf_guard;

	/**
	 * xml 节点构造函数
	 * @param xml_ptr {xml*} xml 树对象，非空
	 * @param node {ACL_XML_NODE*} C 版中的 xml 节点指针
	 */
	xml1_node(xml* xml_ptr, ACL_XML_NODE* node);

	/**
	 * 要求该对象必须是动态创建的
	 */
	~xml1_node(void);

private:
	ACL_XML_NODE *node_;
	ACL_ITER* child_iter_;
	ACL_ITER* attr_iter_;

	xml_node* parent_;
	xml1_node* parent_internal_;
};

class string;

class ACL_CPP_API xml1 : public xml
{
public:
	/**
	 * 构造函数
	 * @param data {const char*} xml 格式的字符串，可以是完整的
	 *  xml 字符串，也可以是部分的 xml 字符串，也可以是空指针，
	 *  无论如何，用户依然可以用部分或完整的 xml 字符串调用 update
	 *  函数，在调用 update 过程中解析 xml；其实，当构造函数的
	 *  的 data 参数非空时，它也会调用 update
	 * @param dbuf_nblock {size_t} 内部所用 dbuf_guard 的初始化参数
	 * @param dbuf_capacity {size_t} 内部所用 dbuf_guard 的初始化参数
	 */
	xml1(const char* data = NULL, size_t dbuf_nblock = 2,
		size_t dbuf_capacity = 100);
	~xml1(void);

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
	/**
	 * 取得 acl 库中的 ACL_XML 对象
	 * @return {ACL_XML*} 该值不可能为空，注意用户可以修改该对象的值，
	 *  但不可以释放该对象
	 */
	ACL_XML* get_xml(void) const
	{
		return xml_;
	}

private:
	ACL_XML *xml_;
	ACL_ITER* iter_;
	xml1_node* root_;
};

} // namespace acl
