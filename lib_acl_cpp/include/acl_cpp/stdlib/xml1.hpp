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
	 * @override 获得属性名
	 * @return {const char*} 属性名，永远不会返回空指针，返回值
	 *  有可能为 "\0"
	 */
	const char* get_name(void) const;

	/**
	 * @override 获得属性值
	 * @return {const char*} 属性值，永远不会返回空指针，返回值
	 *  有可能为 "\0"
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
	 * @override 取得本 XML 节点的标签名
	 * @return {const char*} 返回 XML 节点标签名，如果返回空，则说明
	 *  不存在标签 xxxx，以防万一，调用者需要判断返回值
	 */
	const char* tag_name(void) const;

	/**
	 * @override 如果该 XML 节点的 ID 号属性不存在，则返回空指针
	 * @return {const char*} 当 ID 属性存在时返回对应的值，否则返回空
	 */
	const char* id(void) const;

	/**
	 * @override 返回该 XML 节点的正文内容
	 * @return {const char*} 返回空说明没有正文内容
	 */
	const char* text(void) const;

	/**
	 * @override 返回该 XML 节点的某个属性值
	 * @param name {const char*} 属性名
	 * @return {const char*} 属性值，如果返回空则说明该属性不存在
	 */
	const char* attr_value(const char* name) const;

	/**
	 * @override 遍历节点的所有属性时，需要调用此函数来获得第一个属性对象
	 * @return {const xml_attr*} 返回第一个属性对象，若为空，则表示
	 *  该节点没有属性
	 */
	const xml_attr* first_attr(void) const;

	/**
	 * @override 遍历节点的所有属性时，调用本函数获得下一个属性对象
	 * @return {const xml_attr*} 返回下一下属性对象，若为空，则表示
	 *  遍历完毕
	 */
	const xml_attr* next_attr(void) const;

	/**
	 * @override 添加 XML 节点属性
	 * @param name {const char*} 属性名
	 * @param value {const char*} 属性值
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, const char* value);

	/**
	 * @override 设置 xml 节点的文本内容
	 * @param str {const char*} 字符串内容
	 * @return {xml_node&}
	 */
	xml_node& set_text(const char* str);

	/**
	 * @override 给本 xml 节点添加 xml_node 子节点对象
	 * @param child {xml_node*} 子节点对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {xml_node&} return_child 为 true 返回子节点的引用，
	 *  否则返回本 xml 节点引用
	 */
	xml_node& add_child(xml_node* child, bool return_child = false);

	/**
	 * @override 将本节点及其子节点从 xml 树中分离，其内存将由 xml
	 *  对象统一释放
	 * @return {int} 返回被释放的节点个数
	 */
	int detach(void);

	/**
	 * @override 获得本节点的父级节点对象的引用
	 * @return {xml_node&}
	 */
	xml_node& get_parent(void) const;

	/**
	 * @override 获得本节点第一个子节点，遍历子节点时必须先调用此函数
	 * @return {xml_node*} 返回空表示没有子节点
	 */
	xml_node* first_child(void);

	/**
	 * @override 获得本节点的下一个子节点
	 * @return {xml_node*} 返回空表示遍历过程结束
	 */
	xml_node* next_child(void);

	/**
	 * @override 返回该 XML 节点在整个 XML 树中的深度
	 * @return {int}
	 */
	int depth(void) const;

	/**
	 * @override
	 * @return {bool}
	 */
	bool is_root(void) const;

	/**
	 * @override 返回该 xml 节点的下一级子节点的个数
	 * @return {int} 永远 >= 0
	 */
	int children_count(void) const;

public:
	/**
	 * 取出对应于 ACL 库中的 XML 节点对象
	 * @return {ACL_XML_NODE*} 返回节点对象，注：该节点用户不能单独释放
	 */
	ACL_XML_NODE* get_xml_node(void) const;

	/**
	 * 设置本节点的父节点
	 * @param node {xml_node*} 父节点
	 * @return {xml_node&} 返回本节点引用
	 */
	xml_node& set_parent(xml_node* node);

protected:
	friend class xml1;

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
	 */
	xml1(const char* data = NULL);
	~xml1(void);

	/**
	 * @override 对于非闭合的标签，是否需要忽略闭合字符 '/'，缺省为不忽略
	 * @param on {bool}
	 * @return {xml&}
	 */
	xml& ignore_slash(bool on);

	/**
	 * @override 是否自动进行 xml 解码，缺少为不解码
	 * @param on {bool}
	 * @return {xml&}
	 */
	xml& xml_decode(bool on);

	/**
	 * @override 以流式方式循环调用本函数添加 XML 数据，也可以一次性添加
	 * 完整的 XML 数据，如果是重复使用该 XML 解析器解析多个 XML
	 * 对象，则应该在解析下一个 XML 对象前调用 reset() 方法来清
	 * 除上一次的解析结果
	 * @param data {const char*} xml 数据
	 */
	void update(const char* data);

	/**
	 * @override 重置 XML 解析器状态，该 XML 对象可以用来对多个 XML 数据
	 * 进行解析，在反复使用本 XML 解析器前，需要调用本函数重置
	 * 内部 XML 解析器状态，清除上一次的解析结果
	 */
	void reset(void);

	/**
	 * @override 从解析的 XML 原始数据中仅提取文本部分
	 * @return {const string&} 返回结果缓冲区的引用，该引用是内
	 *  部变量，用户不需要释放
	 */
	const string& getText(void);

	/**
	 * @override 从 XML 对象中取得某个标签名的所有节点集合
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 xml_node 节点数据可以修改，但不能删除该节点，
	 *  因为该库内部有自动删除的机制
	 */
	const std::vector<xml_node*>&
		getElementsByTagName(const char* tag) const;

	/**
	 * @override 从 xml 对象中获得对应标签名的第一个 xml 节点对象
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {xml_node*} 返回空表明该标签对应的 xml 节点不存在
	 */
	xml_node* getFirstElementByTag(const char* tag) const;

	/**
	 * @override 从 xml 对象中获得所有的与给定多级标签名相同的 xml 节点的集合
	 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，如针对 xml 数据：
	 *  <root> <first> <second> <third name="test1"> text1 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test2"> text2 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test3"> text3 </third> </second> </first> ...
	 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合条件的节点
	 * @return {const std::vector<xml_node*>&} 符合条件的 xml 节点集合, 
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 xml_node 节点数据可以修改，但不能删除该节点，
	 *  因为该库内部有自动删除的机制
	 */
	const std::vector<xml_node*>& getElementsByTags(const char* tags) const;

	/**
	 * @override 从 xml 对象中获得指定多级标签名的第一个 xml 节点
	 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，如针对 xml 数据：
	 *  <root> <first> <second> <third name="test1"> text1 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test2"> text2 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test3"> text3 </third> </second> </first> ...
	 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合条件的节点
	 * @return {xml_node*} 返回空表示不存在
	 */
	xml_node* getFirstElementByTags(const char* tags) const;

	/**
	 * @override 从 xml 对象中获得所有与给定属性名 name 的属性值相同的 xml 节点集合
	 * @param value {const char*} 属性名为 name 的属性值
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 xml_node 节点数据可以修改，但不能删除该节点，
	 *  因为该库内部有自动删除的机制
	 */
	const std::vector<xml_node*>&
		getElementsByName(const char* value) const;

	/**
	 * @override 从 xml 对象中获得所有给定属性名及属性值的 xml 节点元素集合
	 * @param name {const char*} 属性名
	 * @param value {const char*} 属性值
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 */
	const std::vector<xml_node*>& getElementsByAttr(
		const char* name, const char* value) const;

	/**
	 * @override 从 xml 对象中获得指定 id 值的 xml 节点元素
	 * @param id {const char*} id 值
	 * @return {xml_node*} xml 节点元素, 若返回 NULL 则表示没有符合条件的
	 *  xml 节点, 返回值不需要释放
	 */
	xml_node* getElementById(const char* id) const;

	/**
	 * @override 创建一个 xml_node 节点对象
	 * @param tag {const char*} 标签名
	 * @param text {const char*} 文本字符串
	 * @return {xml_node*} 新产生的 xml_node 对象不需要用户手工释放，因为在
	 *  xml 对象被释放时这些节点会自动被释放，当然用户也可以在不用时调用
	 *  reset 来释放这些 xml_node 节点对象
	 */
	xml_node& create_node(const char* tag, const char* text = NULL);

	/**
	 * @override 获得根节点对象，但需要注意，该节点为虚节点，里面不存放
	 *  任何数据，它是所有 xml 节点对象的最顶层父对象
	 * @return {xml_node&}
	 */
	xml_node& get_root(void);

	/**
	 * @override 开始遍历该 xml 对象并获得第一个节点
	 * @return {xml_node*} 返回空表示该 xml 对象为空节点
	 *  注：返回的节点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	xml_node* first_node(void);

	/**
	 * @override 遍历该 xml 对象的下一个 xml 节点
	 * @return {xml_node*} 返回空表示遍历完毕
	 *  注：返回的节点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	xml_node* next_node(void);

	/**
	 * @override 将 xml 对象树转成字符串
	 * @param out {string&} 存储转换结果的缓冲区
	 */
	void build_xml(string& out) const;

	/**
	 * @override
	 */
	const char* to_string(size_t* len = NULL) const;

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
