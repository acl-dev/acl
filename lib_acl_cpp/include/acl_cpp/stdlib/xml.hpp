#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <list>
#include "dbuf_pool.hpp"
#include "string.hpp"
#include "pipe_stream.hpp"

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
class istream;

class ACL_CPP_API xml_attr : public dbuf_obj
{
public:
	/**
	 * 获得属性名
	 * @return {const char*} 属性名，永远不会返回空指针，返回值
	 *  有可能为 "\0"
	 */
	virtual const char* get_name(void) const = 0;

	/**
	 * 获得属性值
	 * @return {const char*} 属性值，永远不会返回空指针，返回值
	 *  有可能为 "\0"
	 */
	virtual const char* get_value(void) const = 0;

protected:
	friend class xml_node;

	xml_attr(xml_node* node) : node_(node) {}
	virtual ~xml_attr(void) {}

	xml_node* node_;
};

class ACL_CPP_API xml_node : public dbuf_obj
{
public:
	/**
	 * 取得本 XML 节点的标签名
	 * @return {const char*} 返回 XML 节点标签名，如果返回空，则说明
	 *  不存在标签 xxxx，以防万一，调用者需要判断返回值
	 */
	virtual const char* tag_name(void) const = 0;

	/**
	 * 如果该 XML 节点的 ID 号属性不存在，则返回空指针
	 * @return {const char*} 当 ID 属性存在时返回对应的值，否则返回空
	 */
	virtual const char* id(void) const = 0;

	/**
	 * 返回该 XML 节点的正文内容
	 * @return {const char*} 返回空说明没有正文内容
	 */
	virtual const char* text(void) const = 0;

	/**
	 * 返回该 XML 节点的某个属性值
	 * @param name {const char*} 属性名
	 * @return {const char*} 属性值，如果返回空则说明该属性不存在
	 */
	virtual const char* attr_value(const char* name) const = 0;

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
	virtual const xml_attr* first_attr(void) const = 0;

	/**
	 * 遍历节点的所有属性时，调用本函数获得下一个属性对象
	 * @return {const xml_attr*} 返回下一下属性对象，若为空，则表示
	 *  遍历完毕
	 */
	virtual const xml_attr* next_attr(void) const = 0;

	/**
	 * 添加 XML 节点属性
	 * @param name {const char*} 属性名
	 * @param value {const char*} 属性值
	 * @return {xml_node&}
	 */
	virtual xml_node& add_attr(const char* name, const char* value) = 0;

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
	 * @param append {bool} 添加文本时是采用追加模式还是覆盖模式，如果为追加模式，
	 *  则当原来该节点上有文本内容时，新添加的内容在原文本后面追加，否则则覆盖
	 * @return {xml_node&}
	 */
	virtual xml_node& set_text(const char* str, bool append = false) = 0;

	/**
	 * 设置 xml 节点，同时将流对象中的数据做为该节点的文本内容
	 * @param in {istream&} 输入流对象
	 * @param off {size_t} 对于文件流，则指定要拷贝的数据的起始位置
	 * @param len {size_t} 要拷贝的最大数据量，当为 0 时，则一直拷贝到流结束
	 * @return {xml_node&}
	 */
	virtual xml_node& set_text(istream& in, size_t off = 0,
		size_t len = 0) = 0;

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
	virtual xml_node& add_child(xml_node* child,
		bool return_child = false) = 0;
	
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
	 * @param txt {long long int} 节点中的文本内容，非空字符串
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {xml_node&} return_child 为 true 返回子节点的引用，
	 *  否则返回本 xml 节点引用
	 */
	xml_node& add_child(const char* tag, const char* txt,
		bool return_child = false);

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
	 * 给本 xml 节点添加 xml_node 子节点对象，同时使用输入流中的内容做为节点的文本
	 * @param tag {const char* tag} 子节点对象的标签名
	 * @param in {istream&} 输入流对象
	 * @param off {size_t} 对于文件流，则指定要拷贝的数据的起始位置
	 * @param len {size_t} 要拷贝的最大数据量，当为 0 时，则一直拷贝到流结束
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {xml_node&} return_child 为 true 返回子节点的引用，
	 *  否则返回本 xml 节点引用
	 */
	xml_node& add_child(const char* tag, istream& in,
		size_t off = 0, size_t len = 0, bool return_child = false);

	/**
	 * 获得本节点的父级节点对象的引用
	 * @return {xml_node&}
	 */
	virtual xml_node& get_parent(void) const = 0;

	/**
	 * 设置本节点的父级节点
	 * @param node {xml_node*} 父节点
	 * @return {xml_node&}
	 */
	virtual xml_node& set_parent(xml_node* node) = 0;

	/**
	 * 将本节点及其子节点从 xml 树中分离，其内存将由 xml 对象统一释放
	 * @return {int} 返回被释放的节点个数
	 */
	virtual int detach(void) = 0;

	/**
	 * 获得本节点的第一个子节点，需要遍历子节点时必须首先调用此函数
	 * @return {xml_node*} 返回空表示没有子节点
	 */
	virtual xml_node* first_child(void) = 0;

	/**
	 * 获得本节点的下一个子节点
	 * @return {xml_node*} 返回空表示遍历过程结束
	 */
	virtual xml_node* next_child(void) = 0;

	/**
	 * 返回该 XML 节点在整个 XML 树中的深度
	 * @return {int}
	 */
	virtual int depth(void) const = 0;

	/**
	 * 判断当前节点是否为 xml 对象中的 root 节点
	 * @return {bool}
	 */
	virtual bool is_root(void) const = 0;

	/**
	 * 返回该 xml 节点的下一级子节点的个数
	 * @return {int} 永远 >= 0
	 */
	virtual int children_count(void) const = 0;

	/**
	 * 当在遍历该 xml 节点时，内部会动态产生一些临时 xml_node 对象，调用
	 * 此函数可以清空这些对象，一旦调用此函数进行了清除，则由
	 * first_child/next_child 返回的 xml_node 节点对象将不再可用，否则会
	 * 产生内存非法访问
	 */
	void clear(void);

	/**
	 * 获得 xml 对象的引用
	 * @return {xml&}
	 */
	xml& get_xml(void) const;

protected:
	friend class xml;
	friend class dbuf_guard;

	/**
	 * xml 节点构造函数
	 * @param xml_ptr {xml*} xml 树对象，非空
	 */
	xml_node(xml* xml_ptr);

	/**
	 * 要求该对象必须是动态创建的
	 */
	virtual ~xml_node(void);

protected:
	xml* xml_;
	std::vector<xml_node*> nodes_tmp_;
	std::vector<xml_attr*> attrs_tmp_;
};

class string;

class ACL_CPP_API xml : public pipe_stream, public dbuf_obj
{
public:
	/**
	 * @param dbuf_nblock {size_t} 内部所用 dbuf_guard 的初始化参数
	 * @param dbuf_capacity {size_t} 内部所用 dbuf_guard 的初始化参数
	 */
	xml(size_t dbuf_nblock = 2, size_t dbuf_capacity = 100);
	virtual ~xml(void);

	/**
	 * 对于非闭合的标签，是否需要忽略闭合字符 '/'，缺省为不忽略
	 * @param on {bool}
	 * @return {xml&}
	 */
	virtual xml& ignore_slash(bool on) = 0;

	/**
	 * 解析 xml 对象时，是否自动进行 xml 解码，缺省解码
	 * @param on {bool}
	 * @return {xml&}
	 */
	virtual xml& xml_decode(bool on) = 0;

	/**
	 * 创建 xml 对象时，是否自动进行 xml 编码，缺省编码
	 * @param on {bool}
	 * @return {xml&}
	 */
	virtual xml& xml_encode(bool on) = 0;

	/**
	 * 解析 xml 时是否允许有多个根节点（内部缺省为允许）
	 * @param on {bool}
	 * @retrn {xml&}
	 */
	virtual xml& xml_multi_root(bool on) = 0;

	/**
	 * 以流式方式循环调用本函数添加 XML 数据，也可以一次性添加
	 * 完整的 XML 数据，如果是重复使用该 XML 解析器解析多个 XML
	 * 对象，则应该在解析下一个 XML 对象前调用 reset() 方法来清
	 * 除上一次的解析结果
	 * @param data {const char*} xml 数据
	 * @return {const char*} 当解析完毕时还有剩余数据，则该返回值返回
	 *  剩余的数据; 如果 data 为 '\0'，则说明已经处理完输入的数据
	 */
	virtual const char* update(const char* data) = 0;

	/**
	 * 判断 XML 解析是否完毕
	 * @param root_tag {const char*} 根节点标签名，非 NULL 字符串，用该标签名
	 *  与 xml 对象中最外层的标签名比较是否相同
	 * @return {bool}
	 */
	virtual bool complete(const char* root_tag) = 0;

	/**
	 * 重置 XML 解析器状态，该 XML 对象可以用来对多个 XML 数据
	 * 进行解析，在反复使用本 XML 解析器前，需要调用本函数重置
	 * 内部 XML 解析器状态，清除上一次的解析结果
	 */
	virtual void reset(void) = 0;

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
		getElementsByTagName(const char* tag) const = 0;

	/**
	 * 从 xml 对象中获得对应标签名的第一个 xml 节点对象
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {xml_node*} 返回空表明该标签对应的 xml 节点不存在
	 */
	virtual xml_node* getFirstElementByTag(const char* tag) const = 0;

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
		getElementsByTags(const char* tags) const = 0;

	/**
	 * 从 xml 对象中获得指定多级标签名的第一个 xml 节点
	 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，如针对 xml 数据：
	 *  <root> <first> <second> <third name="test1"> text1 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test2"> text2 </third> </second> </first> ...
	 *  <root> <first> <second> <third name="test3"> text3 </third> </second> </first> ...
	 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合条件的节点
	 * @return {xml_node*} 返回空表示不存在
	 */
	virtual xml_node* getFirstElementByTags(const char* tags) const = 0;

	/**
	 * 从 xml 对象中获得所有与给定属性名 name 的属性值相同的 xml 节点集合
	 * @param value {const char*} 属性名为 name 的属性值
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 xml_node 节点数据可以修改，但不能删除该节点，
	 *  因为该库内部有自动删除的机制
	 */
	virtual const std::vector<xml_node*>&
		getElementsByName(const char* value) const = 0;

	/**
	 * 从 xml 对象中获得所有给定属性名及属性值的 xml 节点元素集合
	 * @param name {const char*} 属性名
	 * @param value {const char*} 属性值
	 * @return {const std::vector<xml_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 */
	virtual const std::vector<xml_node*>& getElementsByAttr(
		const char* name, const char* value) const = 0;

	/**
	 * 从 xml 对象中获得指定 id 值的 xml 节点元素
	 * @param id {const char*} id 值
	 * @return {const xml_node*} xml 节点元素, 若返回 NULL 则表示没有符合
	 *  条件的 xml 节点, 返回值不需要释放
	 */
	virtual xml_node* getElementById(const char* id) const = 0;

	/**
	 * 创建一个 xml_node 节点对象
	 * @param tag {const char*} 标签名
	 * @param txt {const char*} 文本字符串
	 * @return {xml_node*} 新产生的 xml_node 对象不需要用户手工释放，因为
	 *  在 xml 对象被释放时这些节点会自动被释放，当然用户也可以在不用时调
	 *  用 reset 来释放这些 xml_node 节点对象
	 */
	virtual xml_node& create_node(const char* tag,
		const char* txt = NULL) = 0;

	/**
	 * 创建一个 xml_node 节点对象，同时指定输入流中的内容做为节点文本内容
	 * @param tag {const char*} 标签名
	 * @param in {istream&} 输入流对象
	 * @param off {size_t} 对于文件流，则指定要拷贝的数据的起始位置
	 * @param len {size_t} 要拷贝的最大数据量，当为 0 时，则一直拷贝到流结束
	 * @return {xml_node*} 新产生的 xml_node 对象不需要用户手工释放，因为
	 *  在 xml 对象被释放时这些节点会自动被释放，当然用户也可以在不用时调
	 *  用 reset 来释放这些 xml_node 节点对象
	 */
	virtual xml_node& create_node(const char* tag, istream& in,
		size_t off = 0, size_t len = 0) = 0;

	/**
	 * 创建一个 xml_node 节点对象
	 * @param tag {const char*} 标签名
	 * @param number {long long int} 64 位整数
	 * @return {xml_node*} 新产生的 xml_node 对象不需要用户手工释放，因为
	 *  在xml 对象被释放时这些节点会自动被释放，当然用户也可以在不用时调用
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
	virtual xml_node& get_root(void) = 0;

	/**
	 * 开始遍历该 xml 对象并获得第一个节点
	 * @return {xml_node*} 返回空表示该 xml 对象为空节点
	 *  注：返回的节点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	virtual xml_node* first_node(void) = 0;

	/**
	 * 遍历该 xml 对象的下一个 xml 节点
	 * @return {xml_node*} 返回空表示遍历完毕
	 *  注：返回的节点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	virtual xml_node* next_node(void) = 0;

	/**
	 * 将 xml 对象树转成字符串
	 * @param out {string&} 存储转换结果的缓冲区
	 */
	virtual void build_xml(string& out) const { (void) out; };

	/**
	 * 将 xml 对象转换为字符串
	 * @param len {size_t*} 非 NULL 时存放数据长度
	 * @return {const char*} xml 字符串
	 */
	virtual const char* to_string(size_t* len = NULL) const = 0;

	/**
	 * 获得当前 xml 对象已经分配的内存大小总和
	 * @return {size_t}
	 */
	virtual size_t space(void) const = 0;

	/**
	 * 将记录 xml 已分配内存大小的变量清 0
	 */
	virtual void space_clear(void) = 0;

	/**
	 * 获得当前 xml 对象中 xml 节点的总数
	 * @return {size_t}
	 */
	virtual size_t node_count(void) const = 0;

	/**
	 * 获得当前 xml 对象中所有 xml 节点属性的总数
	 * @return {size_t}
	 */
	virtual size_t attr_count(void) const = 0;

public:
	// pipe_stream 虚函数重载

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear(void);

protected:
	dbuf_guard dbuf_;
	std::vector<xml_node*> elements_;
	string* buf_;
	//bool dummyRootAdded_;

	ACL_TOKEN* m_pTokenTree;
	//std::list<xml_node*> nodes_tmp_;
};

} // namespace acl
