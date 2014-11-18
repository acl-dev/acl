#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include <list>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/pipe_stream.hpp"

struct ACL_XML;
struct ACL_XML_NODE;
struct ACL_XML_ATTR;
struct ACL_TOKEN;
struct ACL_ITER;

/**
 * 对 ACL 库中 XML 解析库的封装，方便 C++ 用户使用，如果不太注重性能因素，
 * 可以直接使用该类，如果在服务端执行且非常注重性能，建议直接使用 ACL 库的
 * XML 解析器，因为该类也是调用了 ACL 库中的 XML 解析过程，并且有二次拷贝
 * 过程，可能会稍微影响一些性能，但对于一般的应用这点影响是微不足道的
 */

namespace acl {

class xml;
class xml_node;

class ACL_CPP_API xml_attr
{
public:
	/**
	 * 获得属性名
	 * @return {const char*} 属性名，永远不会返回空指针，返回值
	 *  有可能为 "\0"
	 */
	const char* get_name(void) const;

	/**
	 * 获得属性值
	 * @return {const char*} 属性值，永远不会返回空指针，返回值
	 *  有可能为 "\0"
	 */
	const char* get_value(void) const;
private:
	friend class xml_node;

	xml_node* node_;
	ACL_XML_ATTR* attr_;

	xml_attr(void);
	~xml_attr(void);
};

class ACL_CPP_API xml_node
{
public:
	/**
	 * 取得本 XML 结点的标签名
	 * @return {const char*} 返回 XML 结点标签名，如果返回空，则说明
	 *  不存在标签？xxxx，以防万一，调用者需要判断返回值
	 */
	const char* tag_name(void) const;

	/**
	 * 如果该 XML 结点的 ID 号属性不存在，则返回空指针
	 * @return {const char*} 当 ID 属性存在时返回对应的值，否则返回空
	 */
	const char* id(void) const;

	/**
	 * 返回该 XML 结点的正文内容
	 * @return {const char*} 返回空说明没有正文内容
	 */
	const char* text(void) const;

	/**
	 * 返回该 XML 结点的某个属性值
	 * @param name {const char*} 属性名
	 * @return {const char*} 属性值，如果返回空则说明该属性不存在
	 */
	const char* attr_value(const char* name) const;

	/**
	 * 返回该 XML 结点的某个属性值的便捷写法
	 * @param name {const char*} 属性名
	 * @return {const char*} 属性值，如果返回空则说明该属性不存在
	 */
	const char* operator[](const char* name) const;

	/**
	 * 遍历结点的所有属性时，需要调用此函数来获得第一个属性对象
	 * @return {const xml_attr*} 返回第一个属性对象，若为空，则表示
	 *  该结点没有属性
	 */
	const xml_attr* first_attr(void) const;

	/**
	 * 遍历结点的所有属性时，调用本函数获得下一个属性对象
	 * @return {const xml_attr*} 返回下一下属性对象，若为空，则表示
	 *  遍历完毕
	 */
	const xml_attr* next_attr(void) const;

	/**
	 * 添加 XML 结点属性
	 * @param name {const char*} 属性名
	 * @param value {const char*} 属性值
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, const char* value);

	/**
	 * 添加 XML 结点属性
	 * @param name {const char*} 属性名
	 * @param n {char} 属性值
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, char n);

	/**
	 * 添加 XML 结点属性
	 * @param name {const char*} 属性名
	 * @param n {int} 属性值
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, int n);

	/**
	 * 添加 XML 结点属性
	 * @param name {const char*} 属性名
	 * @param n {size_t} 属性值
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, size_t n);

	/**
	 * 添加 XML 结点属性
	 * @param name {const char*} 属性名
	 * @param n {acl_int64} 属性值
	 * @return {xml_node&}
	 */
#ifdef	WIN32
	xml_node& add_attr(const char* name, __int64 n);
#else
	xml_node& add_attr(const char* name, long long int n);
#endif

	/**
	 * 设置 xml 结点的文本内容
	 * @param str {const char*} 字符串内容
	 * @return {xml_node&}
	 */
	xml_node& set_text(const char* str);

	/**
	 * 给本 xml 结点添加 xml_node 子结点对象
	 * @param child {xml_node*} 子结点对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {xml_node&} return_child 为 true 返回子结点的引用，
	 *  否则返回本 xml 结点引用
	 */
	xml_node& add_child(xml_node* child, bool return_child = false);

	/**
	 * 给本 xml 结点添加 xml_node 子结点对象
	 * @param child {xml_node&} 子结点对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {xml_node&} return_child 为 true 返回子结点的引用，
	 *  否则返回本 xml 结点引用
	 */
	xml_node& add_child(xml_node& child, bool return_child = false);

	/**
	 * 给本 xml 结点添加 xml_node 子结点对象
	 * @param tag {const char* tag} 子结点对象的标签名
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @param str {const char*} 文本字符串
	 * @return {xml_node&} return_child 为 true 返回子结点的引用，
	 *  否则返回本 xml 结点引用
	 */
	xml_node& add_child(const char* tag, bool return_child = false,
		const char* str = NULL);

	/**
	 * 获得本结点的父级结点对象的引用
	 * @return {xml_node&}
	 */
	xml_node& get_parent(void) const;

	/**
	 * 获得本结点的第一个子结点，需要遍历子结点时必须首先调用此函数
	 * @return {xml_node*} 返回空表示没有子结点
	 */
	xml_node* first_child(void);

	/**
	 * 获得本结点的下一个子结点
	 * @return {xml_node*} 返回空表示遍历过程结束
	 */
	xml_node* next_child(void);

	/**
	 * 返回该 XML 结点在整个 XML 树中的深度
	 * @return {int}
	 */
	int   depth(void) const;

	/**
	 * 返回该 xml 结点的下一级子结点的个数
	 * @return {int} 永远 >= 0
	 */
	int   children_count(void) const;

	/**
	 * 获得 xml 对象的引用
	 * @return {xml&}
	 */
	xml& get_xml(void) const;

	/**
	 * 取出对应于 ACL 库中的 XML 结点对象
	 * @return {ACL_XML_NODE*} 返回结点对象，注：该结点用户不能单独释放
	 */
	ACL_XML_NODE* get_xml_node() const;

private:
	friend class xml;

	/**
	 * xml 结点构造函数
	 * @param node {ACL_XML_NODE*} 参见 acl 库 acl_xml.h中的头文件，非空
	 * @param xml_ptr {xml*} xml 树对象，非空
	 */
	xml_node(ACL_XML_NODE* node, xml* xml_ptr);

	/**
	 * 要求该对象必须是动态创建的
	 */
	~xml_node(void);

	/**
	 * 设置 xml 结点
	 * @param node {ACL_XML_NODE*}
	 */
	void set_xml_node(ACL_XML_NODE* node);

private:
	ACL_XML_NODE *node_;
	xml* xml_;
	xml_node* parent_;
	xml_node* parent_saved_;
	xml_node* child_;
	ACL_ITER* child_iter_;
	xml_attr* attr_;
	ACL_ITER* attr_iter_;
};

class string;

class ACL_CPP_API xml : public pipe_stream
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
	xml(const char* data = NULL);
	~xml(void);

	xml& part_word(bool on);
	xml& ignore_slash(bool on);

	/**
	 * 以流式方式循环调用本函数添加 XML 数据，也可以一次性添加
	 * 完整的 XML 数据，如果是重复使用该 XML 解析器解析多个 XML
	 * 对象，则应该在解析下一个 XML 对象前调用 reset() 方法来清
	 * 除上一次的解析结果
	 * @param data {const char*} xml 数据
	 */
	void update(const char* data);

	/**
	 * 重置 XML 解析器状态，该 XML 对象可以用来对多个 XML 数据
	 * 进行解析，在反复使用本 XML 解析器前，需要调用本函数重置
	 * 内部 XML 解析器状态，清除上一次的解析结果
	 */
	void reset(void);

	/**
	 * 从解析的 XML 原始数据中仅提取文本部分
	 * @return {const string&} 返回结果缓冲区的引用，该引用是内
	 *  部变量，用户不需要释放
	 */
	const string& getText(void);

	/**
	 * 从 XML 对象中取得某个标签名的所有结点集合
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 xml_node 结点数据可以修改，但不能删除该结点，
	 *  因为该库内部有自动删除的机制
	 */
	const std::vector<xml_node*>& getElementsByTagName(const char* tag) const;

	/**
	 * 从 xml 对象中获得对应标签名的第一个 xml 结点对象
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {const xml_node*} 返回空表明该标签对应的 xml 结点不存在
	 */
	const xml_node* getFirstElementByTag(const char* tag) const;

	/**
	 * 从 xml 对象中获得所有的与给定多级标签名相同的 xml 结点的集合
	 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，如针对 xml 数据：
	 *  <root> <first> <second> <third name="test1"> text1 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test2"> text2 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test3"> text3 </third> </second> </first> ...
	 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合条件的结点
	 * @return {const std::vector<xml_node*>&} 符合条件的 xml 结点集合, 
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 xml_node 结点数据可以修改，但不能删除该结点，
	 *  因为该库内部有自动删除的机制
	 */
	const std::vector<xml_node*>& getElementsByTags(const char* tags) const;

	/**
	 * 从 xml 对象中获得指定多级标签名的第一个 xml 结点
	 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，如针对 xml 数据：
	 *  <root> <first> <second> <third name="test1"> text1 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test2"> text2 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test3"> text3 </third> </second> </first> ...
	 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合条件的结点
	 * @return {const xml_node*} 返回空表示不存在
	 */
	const xml_node* getFirstElementByTags(const char* tags) const;

	/**
	 * 从 xml 对象中获得所有的与给定属性名 name 的属性值相同的 xml 结点元素集合
	 * @param value {const char*} 属性名为 name 的属性值
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 xml_node 结点数据可以修改，但不能删除该结点，
	 *  因为该库内部有自动删除的机制
	 */
	const std::vector<xml_node*>& getElementsByName(const char* value) const;

	/**
	 * 从 xml 对象中获得所有给定属性名及属性值的 xml 结点元素集合
	 * @param name {const char*} 属性名
	 * @param value {const char*} 属性值
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 */
	const std::vector<xml_node*>& getElementsByAttr(
		const char* name, const char* value) const;

	/**
	 * 从 xml 对象中获得指定 id 值的 xml 结点元素
	 * @param id {const char*} id 值
	 * @return {const xml_node*} xml 结点元素, 若返回 NULL 则表示没有符合
	 *  条件的 xml 结点, 返回值不需要释放
	 */
	const xml_node* getElementById(const char* id) const;

	/**
	 * 取得 acl 库中的 ACL_XML 对象
	 * @return {ACL_XML*} 该值不可能为空，注意用户可以修改该对象的值，
	 *  但不可以释放该对象
	 */
	ACL_XML* get_xml(void) const;

	/**
	 * 创建一个 xml_node 结点对象
	 * @param tag {const char*} 标签名
	 * @param text {const char*} 文本字符串
	 * @return {xml_node*} 新产生的 xml_node 对象不需要用户手工释放，因为在
	 *  xml 对象被释放时这些结点会自动被释放，当然用户也可以在不用时调用
	 *  reset 来释放这些 xml_node 结点对象
	 */
	xml_node& create_node(const char* tag, const char* text = NULL);

	/**
	 * 获得根结点对象，但需要注意，该结点为虚结点，里面不存放任何数据，
	 * 它是所有 xml 结点对象的最顶层父对象
	 * @return {xml_node&}
	 */
	xml_node& get_root();

	/**
	 * 开始遍历该 xml 对象并获得第一个结点
	 * @return {xml_node*} 返回空表示该 xml 对象为空结点
	 *  注：返回的结点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	xml_node* first_node(void);

	/**
	 * 遍历该 xml 对象的下一个 xml 结点
	 * @return {xml_node*} 返回空表示遍历完毕
	 *  注：返回的结点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	xml_node* next_node(void);

	/**
	 * 将 xml 对象树转成字符串
	 * @param out {string&} 存储转换结果的缓冲区
	 */
	void build_xml(string& out);

	// pipe_stream 虚函数重载

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear(void);

private:
	ACL_XML *xml_;
	xml_node* node_;
	xml_node* root_;
	std::vector<xml_node*> elements_;
	string* buf_;
	//bool dummyRootAdded_;

	ACL_TOKEN* m_pTokenTree;
	std::list<xml_node*> nodes_;
	ACL_ITER* iter_;
};

} // namespace acl
