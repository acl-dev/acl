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
	virtual const char* get_name(void) const;

	/**
	 * 获得属性值
	 * @return {const char*} 属性值，永远不会返回空指针，返回值
	 *  有可能为 "\0"
	 */
	virtual const char* get_value(void) const;

protected:
	xml_attr(void);
	virtual ~xml_attr(void);

	xml_node* node_;

private:
	friend class xml_node;

	ACL_XML_ATTR* attr_;
};

class ACL_CPP_API xml_node
{
public:
	/**
	 * 取得本 XML 节点的标签名
	 * @return {const char*} 返回 XML 节点标签名，如果返回空，则说明
	 *  不存在标签 xxxx，以防万一，调用者需要判断返回值
	 */
	virtual const char* tag_name(void) const;

	/**
	 * 如果该 XML 节点的 ID 号属性不存在，则返回空指针
	 * @return {const char*} 当 ID 属性存在时返回对应的值，否则返回空
	 */
	virtual const char* id(void) const;

	/**
	 * 返回该 XML 节点的正文内容
	 * @return {const char*} 返回空说明没有正文内容
	 */
	virtual const char* text(void) const;

	/**
	 * 返回该 XML 节点的某个属性值
	 * @param name {const char*} 属性名
	 * @return {const char*} 属性值，如果返回空则说明该属性不存在
	 */
	virtual const char* attr_value(const char* name) const;

	/**
	 * 返回该 XML 节点的某个属性值的便捷写法
	 * @param name {const char*} 属性名
	 * @return {const char*} 属性值，如果返回空则说明该属性不存在
	 */
	const char* operator[](const char* name) const;

	/**
	 * 遍历节点的所有属性时，需要调用此函数来获得第一个属性对象
	 * @return {const xml_attr*} 返回第一个属性对象，若为空，则表示
	 *  该节点没有属性
	 */
	virtual const xml_attr* first_attr(void) const;

	/**
	 * 遍历节点的所有属性时，调用本函数获得下一个属性对象
	 * @return {const xml_attr*} 返回下一下属性对象，若为空，则表示
	 *  遍历完毕
	 */
	virtual const xml_attr* next_attr(void) const;

	/**
	 * 添加 XML 节点属性
	 * @param name {const char*} 属性名
	 * @param value {const char*} 属性值
	 * @return {xml_node&}
	 */
	virtual xml_node& add_attr(const char* name, const char* value);

	/**
	 * 添加 XML 节点属性
	 * @param name {const char*} 属性名
	 * @param n {char} 属性值
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, char n);

	/**
	 * 添加 XML 节点属性
	 * @param name {const char*} 属性名
	 * @param n {int} 属性值
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, int n);

	/**
	 * 添加 XML 节点属性
	 * @param name {const char*} 属性名
	 * @param n {size_t} 属性值
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, size_t n);

	/**
	 * 添加 XML 节点属性
	 * @param name {const char*} 属性名
	 * @param n {acl_int64} 属性值
	 * @return {xml_node&}
	 */
#if defined(_WIN32) || defined(_WIN64)
	xml_node& add_attr(const char* name, __int64 n);
#else
	xml_node& add_attr(const char* name, long long int n);
#endif

	/**
	 * 设置 xml 节点的文本内容
	 * @param str {const char*} 字符串内容
	 * @return {xml_node&}
	 */
	virtual xml_node& set_text(const char* str);

	/**
	 * 设置 xml 节点的文本内容
	 * @param number {long long int} 64 位整数
	 * @return {xml_node&}
	 */
#if defined(_WIN32) || defined(_WIN64)
	xml_node& set_text(__int64 number);
#else
	xml_node& set_text(long long int number);
#endif

	/**
	 * 给本 xml 节点添加 xml_node 子节点对象
	 * @param child {xml_node*} 子节点对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {xml_node&} return_child 为 true 返回子节点的引用，
	 *  否则返回本 xml 节点引用
	 */
	virtual xml_node& add_child(xml_node* child, bool return_child = false);

	/**
	 * 给本 xml 节点添加 xml_node 子节点对象
	 * @param child {xml_node&} 子节点对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {xml_node&} return_child 为 true 返回子节点的引用，
	 *  否则返回本 xml 节点引用
	 */
	xml_node& add_child(xml_node& child, bool return_child = false);

	/**
	 * 给本 xml 节点添加 xml_node 子节点对象
	 * @param tag {const char* tag} 子节点对象的标签名
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @param str {const char*} 文本字符串
	 * @return {xml_node&} return_child 为 true 返回子节点的引用，
	 *  否则返回本 xml 节点引用
	 */
	xml_node& add_child(const char* tag, bool return_child = false,
		const char* str = NULL);

	/**
	 * 给本 xml 节点添加 xml_node 子节点对象
	 * @param tag {const char* tag} 子节点对象的标签名
	 * @param number {long long int} 64 位整数
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {xml_node&} return_child 为 true 返回子节点的引用，
	 *  否则返回本 xml 节点引用
	 */
#if defined(_WIN32) || defined(_WIN64)
	xml_node& add_child(const char* tag, __int64 number,
		bool return_child = false);
#else
	xml_node& add_child(const char* tag, long long int number,
		bool return_child = false);
#endif

	/**
	 * 获得本节点的父级节点对象的引用
	 * @return {xml_node&}
	 */
	virtual xml_node& get_parent(void) const;

	/**
	 * 获得本节点的第一个子节点，需要遍历子节点时必须首先调用此函数
	 * @return {xml_node*} 返回空表示没有子节点
	 */
	virtual xml_node* first_child(void);

	/**
	 * 获得本节点的下一个子节点
	 * @return {xml_node*} 返回空表示遍历过程结束
	 */
	virtual xml_node* next_child(void);

	/**
	 * 返回该 XML 节点在整个 XML 树中的深度
	 * @return {int}
	 */
	virtual int depth(void) const;

	/**
	 * 返回该 xml 节点的下一级子节点的个数
	 * @return {int} 永远 >= 0
	 */
	virtual int children_count(void) const;

	/**
	 * 当在遍历该 xml 节点时，内部会动态产生一些临时 xml_node 对象，调用
	 * 此函数可以清空这些对象，一旦调用此函数进行了清除，则由
	 * first_child/next_child 返回的 xml_node 节点对象将不再可用，否则会
	 * 产生内存非法访问
	 */
	void clear();

	/**
	 * 获得 xml 对象的引用
	 * @return {xml&}
	 */
	xml& get_xml(void) const;

public:
	/**
	 * 取出对应于 ACL 库中的 XML 节点对象
	 * @return {ACL_XML_NODE*} 返回节点对象，注：该节点用户不能单独释放
	 */
	ACL_XML_NODE* get_xml_node(void) const;

protected:
	friend class xml;

	/**
	 * xml 节点构造函数
	 * @param xml_ptr {xml*} xml 树对象，非空
	 */
	xml_node(xml* xml_ptr);

	/**
	 * 要求该对象必须是动态创建的
	 */
	virtual ~xml_node(void);

	/**
	 * 设置 xml 节点
	 * @param node {ACL_XML_NODE*}
	 */
	void set_xml_node(ACL_XML_NODE* node);

protected:
	xml* xml_;
	xml_node* parent_;
	xml_node* parent_saved_;
	ACL_ITER* child_iter_;
	ACL_ITER* attr_iter_;
	std::vector<xml_node*> nodes_tmp_;
	std::vector<xml_attr*> attrs_tmp_;

private:
	ACL_XML_NODE *node_;
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
	 * @param subclass {bool} 是否为子类实例，即如果该类实现是由
	 *  子类构造，则需要填 true，否则该 xml 解析器直接由本类构造
	 */
	xml(const char* data = NULL, bool subclass = false);
	virtual ~xml(void);

	/**
	 * 对于非闭合的标签，是否需要忽略闭合字符 '/'，缺省为不忽略
	 * @param on {bool}
	 * @return {xml&}
	 */
	virtual xml& ignore_slash(bool on);

	/**
	 * 是否自动进行 xml 解码，缺少为不解码
	 * @param on {bool}
	 * @return {xml&}
	 */
	virtual xml& xml_decode(bool on);

	/**
	 * 以流式方式循环调用本函数添加 XML 数据，也可以一次性添加
	 * 完整的 XML 数据，如果是重复使用该 XML 解析器解析多个 XML
	 * 对象，则应该在解析下一个 XML 对象前调用 reset() 方法来清
	 * 除上一次的解析结果
	 * @param data {const char*} xml 数据
	 */
	virtual void update(const char* data);

	/**
	 * 重置 XML 解析器状态，该 XML 对象可以用来对多个 XML 数据
	 * 进行解析，在反复使用本 XML 解析器前，需要调用本函数重置
	 * 内部 XML 解析器状态，清除上一次的解析结果
	 */
	virtual void reset(void);

	/**
	 * 从解析的 XML 原始数据中仅提取文本部分
	 * @return {const string&} 返回结果缓冲区的引用，该引用是内
	 *  部变量，用户不需要释放
	 */
	virtual const string& getText(void);

	/**
	 * 从 XML 对象中取得某个标签名的所有节点集合
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 xml_node 节点数据可以修改，但不能删除该节点，
	 *  因为该库内部有自动删除的机制
	 */
	virtual const std::vector<xml_node*>&
		getElementsByTagName(const char* tag) const;

	/**
	 * 从 xml 对象中获得对应标签名的第一个 xml 节点对象
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {const xml_node*} 返回空表明该标签对应的 xml 节点不存在
	 */
	virtual const xml_node* getFirstElementByTag(const char* tag) const;

	/**
	 * 从 xml 对象中获得所有的与给定多级标签名相同的 xml 节点的集合
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
	virtual const std::vector<xml_node*>&
		getElementsByTags(const char* tags) const;

	/**
	 * 从 xml 对象中获得指定多级标签名的第一个 xml 节点
	 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，如针对 xml 数据：
	 *  <root> <first> <second> <third name="test1"> text1 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test2"> text2 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test3"> text3 </third> </second> </first> ...
	 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合条件的节点
	 * @return {const xml_node*} 返回空表示不存在
	 */
	virtual const xml_node* getFirstElementByTags(const char* tags) const;

	/**
	 * 从 xml 对象中获得所有与给定属性名 name 的属性值相同的 xml 节点集合
	 * @param value {const char*} 属性名为 name 的属性值
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 xml_node 节点数据可以修改，但不能删除该节点，
	 *  因为该库内部有自动删除的机制
	 */
	virtual const std::vector<xml_node*>&
		getElementsByName(const char* value) const;

	/**
	 * 从 xml 对象中获得所有给定属性名及属性值的 xml 节点元素集合
	 * @param name {const char*} 属性名
	 * @param value {const char*} 属性值
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 */
	virtual const std::vector<xml_node*>& getElementsByAttr(
		const char* name, const char* value) const;

	/**
	 * 从 xml 对象中获得指定 id 值的 xml 节点元素
	 * @param id {const char*} id 值
	 * @return {const xml_node*} xml 节点元素, 若返回 NULL 则表示没有符合
	 *  条件的 xml 节点, 返回值不需要释放
	 */
	virtual const xml_node* getElementById(const char* id) const;

	/**
	 * 创建一个 xml_node 节点对象
	 * @param tag {const char*} 标签名
	 * @param text {const char*} 文本字符串
	 * @return {xml_node*} 新产生的 xml_node 对象不需要用户手工释放，因为在
	 *  xml 对象被释放时这些节点会自动被释放，当然用户也可以在不用时调用
	 *  reset 来释放这些 xml_node 节点对象
	 */
	virtual xml_node& create_node(const char* tag, const char* text = NULL);

	/**
	 * 创建一个 xml_node 节点对象
	 * @param tag {const char*} 标签名
	 * @param number {long long int} 64 位整数
	 * @return {xml_node*} 新产生的 xml_node 对象不需要用户手工释放，因为在
	 *  xml 对象被释放时这些节点会自动被释放，当然用户也可以在不用时调用
	 *  reset 来释放这些 xml_node 节点对象
	 */
#if defined(_WIN32) || defined(_WIN64)
	xml_node& create_node(const char* tag, __int64 number);
#else
	xml_node& create_node(const char* tag, long long int number);
#endif

	/**
	 * 获得根节点对象，但需要注意，该节点为虚节点，里面不存放任何数据，
	 * 它是所有 xml 节点对象的最顶层父对象
	 * @return {xml_node&}
	 */
	virtual xml_node& get_root(void);

	/**
	 * 开始遍历该 xml 对象并获得第一个节点
	 * @return {xml_node*} 返回空表示该 xml 对象为空节点
	 *  注：返回的节点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	virtual xml_node* first_node(void);

	/**
	 * 遍历该 xml 对象的下一个 xml 节点
	 * @return {xml_node*} 返回空表示遍历完毕
	 *  注：返回的节点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	virtual xml_node* next_node(void);

	/**
	 * 将 xml 对象树转成字符串
	 * @param out {string&} 存储转换结果的缓冲区
	 */
	virtual void build_xml(string& out) const;

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

public:
	// pipe_stream 虚函数重载

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear(void);

protected:
	xml_node* root_;
	std::vector<xml_node*> elements_;
	string* buf_;
	//bool dummyRootAdded_;

	ACL_TOKEN* m_pTokenTree;
	std::list<xml_node*> nodes_tmp_;
	ACL_ITER* iter_;

private:
	ACL_XML *xml_;
};

} // namespace acl
