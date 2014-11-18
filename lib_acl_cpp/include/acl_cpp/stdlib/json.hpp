#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>
#include <vector>
#include "acl_cpp/stdlib/pipe_stream.hpp"

struct ACL_JSON_NODE;
struct ACL_JSON;
struct ACL_ITER;

/**
 * 对 ACL 库中 json 解析库的封装，方便 C++ 用户使用，如果不太注重性能因素，
 * 可以直接使用该类，如果在服务端执行且非常注重性能，建议直接使用 ACL 库的
 * json 解析器，因为该类也是调用了 ACL 库中的 json 解析过程，并且有二次拷贝
 * 过程，可能会稍微影响一些性能，但对于一般的应用这点影响是微不足道的
 */

namespace acl {

class string;
class json;

/**
 * json 结点，该类对象必须以 json.create_node() 方式创建
 */
class ACL_CPP_API json_node
{
public:
	/**
	 * 取得本 json 结点的标签名
	 * @return {const char*} 返回 json 结点标签名，如果返回空，则说明
	 *  调用者需要判断返回值
	 */
	const char* tag_name(void) const;

	/**
	 * 返回该 json 结点的文本标签值，当该值为布尔型或数值型时调用者可
	 * 自行进行转换
	 * @return {const char*} 返回空说明没有文本标签值
	 */
	const char* get_text(void) const;

	/**
	 * 当该 json 结点存在子结点时，返回本 json 结点标签对应的 json 子结点
	 * @param {const json_node*} 返回 NULL 说明不存在子结点
	 *  注：get_text 与 get_obj 不会同时返回非 NULL
	 */
	json_node* get_obj(void) const;

	/**
	 * 当该 json 结点有标签时，本函数用来新的标签值覆盖旧的标签名
	 * @param name {const char*} 新的标签值，为非空字符串
	 * @return {bool} 返回 false 表示该结点没有标签或输入空串，没有进行替换
	 */
	bool set_tag(const char* name);

	/**
	 * 当该 json 结点为叶结点时，本函数用来替换结点的文本值
	 * @param text {const char*} 新的叶结点文本值，为非空字符串
	 * @return {bool} 返回 false 表示该结点非叶结点或输入非法
	 */
	bool set_text(const char* text);

	/**
	 * 将当前 json 结点转换成 json 字符串(包含本 json 结点及其子结点)
	 * @return {const char*}
	 */
	const string& to_string(void);

	/**
	 * 给本 json 结点添加 json_node 子结点对象
	 * @param child {json_node*} 子结点对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时返回子结点的引用，
	 *  否则返回本 json 结点对象的引用
	 */
	json_node& add_child(json_node* child, bool return_child = false);

	/**
	 * 给本 json 结点添加 json_node 子结点对象
	 * @param child {json_node&} 子结点对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时返回子结点的引用，
	 *  否则返回本 json 结点对象的引用
	 */
	json_node& add_child(json_node& child, bool return_child = false);

	/**
	 * 创建一个 json 结点对象，并将之添加为本 json 结点的子结点
	 * @param as_array {bool} 是否数组对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时创建的新结点的引用，
	 *  否则返回本 json 结点对象的引用
	 */
	json_node& add_array(bool return_child = false);
	json_node& add_child(bool as_array = false, bool return_child = false);

	/**
	 * 创建一个字符串类型的 json 结点对象，并将之添加为本 json 结点的子结点
	 * @param tag {const char*} 标签名
	 * @param value {const char*} 标签值
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时创建的新结点的引用，
	 *  否则返回本 json 结点对象的引用
	 * 注：此处的 add_text 和 add_child 是同样的功能
	 */
	json_node& add_text(const char* tag, const char* value,
		bool return_child = false);

	json_node& add_child(const char* tag, const char* value,
		bool return_child = false);

	/**
	 * 创建一个int64 类型的 json 结点对象，并将之添加为本 json 结点的子结点
	 * @param tag {const char*} 标签名
	 * @param value {int64} 标签值
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时创建的新结点的引用，
	 *  否则返回本 json 结点对象的引用
	 */
#ifdef WIN32
	json_node& add_number(const char* tag, __int64 value,
		bool return_child = false);
#else
	json_node& add_number(const char* tag, long long int value,
		bool return_child = false);
#endif

	/**
	 * 创建一个布尔类型的 json 结点对象，并将之添加为本 json 结点的子结点
	 * @param tag {const char*} 标签名
	 * @param value {bool} 标签值
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时创建的新结点的引用，
	 *  否则返回本 json 结点对象的引用
	 */
	json_node& add_bool(const char* tag, bool value,
		bool return_child = false);

	/**
	 * 创建一个 json 字符串对象，并将之添加为本 json 结点的子结点
	 * @param text {const char*} 文本字符串
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时创建的新结点的引用，
	 *  否则返回本 json 结点对象的引用
	 */
	json_node& add_array_text(const char* text, bool return_child = false);

	ACL_CPP_DEPRECATED_FOR("add_text")
	json_node& add_child(const char* text, bool return_child = false);

	/**
	 * 创建一个 json 数字对象，并将之添加为本 json 结点的子结点
	 * @param value {acl_int64} 数字值
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时创建的新结点的引用，
	 *  否则返回本 json 结点对象的引用
	 */
#ifdef WIN32
	json_node& add_array_number(__int64 value, bool return_child = false);
#else
	json_node& add_array_number(long long int value, bool return_child = false);
#endif

	/**
	 * 创建一个 json 布尔对象，并将之添加为本 json 结点的子结点
	 * @param value {bool} 布尔值
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时创建的新结点的引用，
	 *  否则返回本 json 结点对象的引用
	 */
	json_node& add_array_bool(bool value, bool return_child = false);

	/**
	 * 创建一个 json 结点对象，并将之添加为本 json 结点的子结点
	 * @param tag {const char*} 标签名
	 * @param node {json_node*} 标签值指针
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时创建的新结点的引用，
	 *  否则返回本 json 结点对象的引用
	 */
	json_node& add_child(const char* tag, json_node* node,
		bool return_child = false);

	/**
	 * 创建一个 json 结点对象，并将之添加为本 json 结点的子结点
	 * @param tag {const char*} 标签名
	 * @param node {json_node&} 标签值引用
	 * @param return_child {bool} 是否需要本函数返回新创建的子结点的引用
	 * @return {json_node&} return_child 为 true 时创建的新结点的引用，
	 *  否则返回本 json 结点对象的引用
	 */
	json_node& add_child(const char* tag, json_node& node,
		bool return_child = false);

	/**
	 * @return {json_node&} 返回本结点的父结点引用
	 */
	json_node& get_parent(void) const;

	/**
	 * 获得本结点的第一个子结点，需要遍历子结点时必须首先调用此函数
	 * @return {json_node*} 返回空表示没有子结点
	 */
	json_node* first_child(void);

	/**
	 * 获得本结点的下一个子结点
	 * @return {json_node*} 返回空表示遍历过程结束
	 */
	json_node* next_child(void);

	/**
	 * 返回该 json 结点在整个 json 树中的深度
	 * @return {int}
	 */
	int   depth(void) const;

	/**
	 * 返回该 json 结点的下一级子结点的个数
	 * @return {int} 永远 >= 0
	 */
	int   children_count(void) const;

	/**
	 * 获得 json 对象的引用
	 * @return {json&}
	 */
	json& get_json(void) const;

	/**
	 * 取出对应于 ACL 库中的 json 结点对象
	 * @return {ACL_JSON_NODE*} 返回结点对象，注：该结点用户不能单独释放
	 */
	ACL_JSON_NODE* get_json_node() const;

private:
	friend class json;
	/**
	 * 构造函数，要求该对象必须只能由 json 对象创建
	 * @param node {ACL_JSON_NODE*} ACL 库中的 ACL_JSON_NODE 结构对象
	 */
	json_node(ACL_JSON_NODE* node, json* json_ptr);

	/**
	 * 要求该对象必须是动态创建的
	 */
	~json_node(void);

	/**
	 * 设置 json 结点
	 * @param node {ACL_JSON_NODE*}
	 */
	void set_json_node(ACL_JSON_NODE* node);

private:
	ACL_JSON_NODE* node_me_;
	json* json_;
	json_node* parent_;
	json_node* parent_saved_;
	json_node* child_;
	ACL_ITER* iter_;
	string* buf_;
	json_node* obj_;
};

class ACL_CPP_API json : public pipe_stream
{
public:
	/**
	 * 构造函数，可用于解析 json 字符串或生成 json 对象
	 * @param data {const char*} json 格式的字符串，可以是完整的
	 *  json 字符串，也可以是部分的 json 字符串，也可以是空指针，
	 *  无论如何，用户依然可以用部分或完整的 json 字符串调用 update
	 *  函数，在调用 update 过程中解析 json；其实，当构造函数的
	 *  的 data 参数非空时，它也会调用 update
	 */
	json(const char* data = NULL);

	/**
	 * 根据一个 json 对象中的一个 json 结点构造一个新的 json 对象
	 * @param node {const json_node&} 源 json 对象中的一个 json 结点
	 */
	json(const json_node& node);

	~json(void);

	/**
	 * 设置是否在解析时自动处理半个汉字的情形
	 * @param on {bool}
	 * @return {json&}
	 */
	json& part_word(bool on);

	/**
	 * 以流式方式循环调用本函数添加 json 数据，也可以一次性添加
	 * 完整的 json 数据，如果是重复使用该 json 解析器解析多个 json
	 * 对象，则应该在解析下一个 json 对象前调用 reset() 方法来清
	 * 除上一次的解析结果
	 * @param data {const char*} json 数据
	 */
	void update(const char* data);

	/**
	 * 重置 json 解析器状态，该 json 对象可以用来对多个 json 数据
	 * 进行解析，在反复使用本 json 解析器前，需要调用本函数重置
	 * 内部 json 解析器状态，清除上一次的解析结果
	 */
	void reset(void);

	/**
	 * 从 json 对象中取得某个标签名的所有结点集合
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {const std::vector<json_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 json_node 结点数据可以修改，但不能删除该结点，
	 *  因为该库内部有自动删除的机制
	 */
	const std::vector<json_node*>& getElementsByTagName(const char* tag) const;

	/**
	 * 从 json 对象中获得所有的与给定多级标签名相同的 json 结点的集合
	 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，
	 *  如针对 json 数据：
	 *  { 'root': [
	 *      'first': { 'second': { 'third': 'test1' } },
	 *      'first': { 'second': { 'third': 'test2' } },
	 *      'first': { 'second': { 'third': 'test3' } }
	 *    ]
	 *  }
	 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合
	 *  条件的结点
	 * @return {const std::vector<json_node*>&} 符合条件的 json 结点集合, 
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的数组中的 json_node 结点数据可以修改，但不能删除该结点，
	 *  因为该库内部有自动删除的机制
	 */
	const std::vector<json_node*>& getElementsByTags(const char* tags) const;

	/**
	 * 取得 acl 库中的 ACL_JSON 对象
	 * @return {ACL_JSON*} 该值不可能为空，注意用户可以修改该对象的值，
	 *  但不可以释放该对象
	 */
	ACL_JSON* get_json(void) const;

	/**
	 * 创建一个 json_node 叶结点对象，该结点对象的格式为：
	 * "tag_name": "tag_value"
	 * @param tag {const char*} 标签名
	 * @param value {const char*} 标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些结点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 结点对象
	 */
	json_node& create_node(const char* tag, const char* value);

	/**
	 * 创建一个 json_node 叶结点对象，该结点对象的格式为：
	 * "tag_name": tag_value
	 * @param tag {const char*} 标签名
	 * @param value {int64} 标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些结点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 结点对象
	 */
#ifdef WIN32
	json_node& create_node(const char* tag, __int64 value);
#else
	json_node& create_node(const char* tag, long long int value);
#endif

	/**
	 * 创建一个 json_node 叶结点对象，该结点对象的格式为：
	 * "tag_name": true|false
	 * @param tag {const char*} 标签名
	 * @param value {bool} 标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些结点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 结点对象
	 */
	json_node& create_node(const char* tag, bool value);

	/**
	 * 创建一个 json_node 叶结点字符串对象，该结点对象的格式为："string"
	 * 按 json 规范，该结点只能加入至数组对象中
	 * @param text {const char*} 文本字符串
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些结点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 结点对象
	 */
	json_node& create_array_text(const char* text);

	ACL_CPP_DEPRECATED_FOR("create_array_text")
	json_node& create_node(const char* text);

	/**
	 * 创建一个 json_node 叶结点数值对象
	 * 按 json 规范，该结点只能加入至数组对象中
	 * @param value {acl_int64} 数值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些结点会自动被释放，当然用户也可以在
	 * 不用时调用 reset 来释放这些 json_node 结点对象
	 */
#ifdef WIN32
	json_node& create_array_number(__int64 value);
#else
	json_node& create_array_number(long long int value);
#endif

	/**
	 * 创建一个 json_node 叶结点布尔对象
	 * 按 json 规范，该结点只能加入至数组对象中
	 * @param value {bool} 布尔值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些结点会自动被释放，当然用户也可以在
	 * 不用时调用 reset 来释放这些 json_node 结点对象
	 */
	json_node& create_array_bool(bool value);

	/**
	 * 创建一个 json_node 结点容器对象，该对象没有标签,
	 * 该结点对象的格式为："{}" 或数组对象 "[]"
	 * @param as_array {bool} 是否数组对象
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些结点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 结点对象
	 */
	json_node& create_node(bool as_array = false);
	json_node& create_array();

	/**
	 * 创建一个 json_node 结点对象，该结点对象的格式为：tag_name: {}
	 * 或 tag_name: []
	 * @param tag {const char*} 标签名
	 * @param node {json_node*} json 结点对象作为标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些结点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 结点对象
	 */
	json_node& create_node(const char* tag, json_node* node);

	/**
	 * 创建一个 json_node 结点对象，该结点对象的格式为：tag_name: {}
	 * 或 tag_name: []
	 * @param tag {const char*} 标签名
	 * @param node {json_node&} json 结点对象作为标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些结点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 结点对象
	 */
	json_node& create_node(const char* tag, json_node& node);

	/**
	 * 将一个 json 对象中的一个 json 结点复制至任一 json 对象中的一个
	 * 新 json 结点中并返回该新的 json 结点
	 * @param node {json_node*} 源 json 对象的一个 json 结点
	 * @return {json_node&} 当前目标 json 对象中新创建的 json 结点
	 */
	json_node& duplicate_node(const json_node* node);

	/**
	 * 将一个 json 对象中的一个 json 结点复制至任一 json 对象中的一个
	 * 新 json 结点中并返回该新的 json 结点
	 * @param node {json_node&} 源 json 对象的一个 json 结点
	 * @return {json_node&} 当前目标 json 对象中新创建的 json 结点
	 */
	json_node& duplicate_node(const json_node& node);

	/**
	 * 获得根结点对象
	 * @return {json_node&}
	 */
	json_node& get_root();

	/**
	 * 开始遍历该 json 对象并获得第一个结点
	 * @return {json_node*} 返回空表示该 json 对象为空结点
	 *  注：返回的结点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	json_node* first_node(void);

	/**
	 * 遍历该 json 对象的下一个 json 结点
	 * @return {json_node*} 返回空表示遍历完毕
	 *  注：返回的结点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	json_node* next_node(void);

	/**
	 * 将 json 对象树转成字符串
	 * @param out {string&} 存储转换结果的缓冲区
	 */
	void build_json(string& out);

	/**
	 * 将 json 对象树转换成 json 字符串
	 * @return {const string&}
	 */
	const string& to_string();

	// pipe_stream 虚函数重载

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear(void);
private:
	// 对应于 acl 库中的 ACL_JSON 对象
	ACL_JSON *json_;
	// json 对象树中的根结点对象
	json_node* root_;
	// 由该 json 容器分配的 json 结点集合
	std::list<json_node*> nodes_;
	// 临时的 json 结点对象
	json_node* node_tmp_;
	// 临时的 json 结点查询结果集
	std::vector<json_node*> nodes_tmp_;
	// 缓冲区
	string* buf_;
	ACL_ITER* iter_;
};

} // namespace acl
