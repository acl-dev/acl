#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>
#include <vector>
#include "acl_cpp/stdlib/dbuf_pool.hpp"
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
 * json 节点，该类对象必须以 json.create_node() 方式创建
 */
class ACL_CPP_API json_node : public dbuf_obj
{
public:
	/**
	 * 取得本 json 节点的标签名
	 * @return {const char*} 返回 json 节点标签名，如果返回空，则说明
	 *  调用者需要判断返回值
	 */
	const char* tag_name(void) const;

	/**
	 * 返回该 json 节点的文本标签值，当该值为布尔型或数值型时调用者可
	 * 自行进行转换
	 * @return {const char*} 返回空说明没有文本标签值
	 */
	const char* get_text(void) const;

	/**
	 * 当该 json 节点存在子节点时，返回本 json 节点标签对应的 json 子节点
	 * @param {const json_node*} 返回 NULL 说明不存在子节点
	 *  注：get_text 与 get_obj 不会同时返回非 NULL
	 */
	json_node* get_obj(void) const;

	/**
	 * 当 json 节点为字符串类型时，该函数返回字符串内容
	 * @return {const char*} 返回 NULL 表示该节点非字符串类型
	 */
	const char* get_string(void) const;

	/**
	 * 当 json 节点为长整型类型时，该函数返回长整型值的指针地址
	 * @return {const long long int*} 当返回 NULL 时表示该对象非长整型类型
	 */
#if defined(_WIN32) || defined(_WIN64)
	const __int64* get_int64(void) const;
#else
	const long long int* get_int64(void) const;
#endif

	/**
	 * 当 json 节点为浮点类型时，该函数返回长整型值的指针地址
	 * @return {const double*} 当返回 NULL 时表示该对象非浮点类型
	 */
	const double *get_double(void) const;

	/**
	 * 当 json 节点为布尔类型时，该函数返回布尔值的指针地址
	 * @return {bool*} 当返回 NULL 时表示该对象非布尔类型
	 */
	const bool* get_bool(void) const;

	/**
	 * 判断本节点数据是否为字符串类型
	 * @return {bool}
	 */
	bool is_string(void) const;

	/**
	 * 判断本节点数据是否为数字类型
	 * @return {bool}
	 */
	bool is_number(void) const;

	/**
	 * 判断本节点数据是否为浮点类型
	 * @return {bool}
	 */
	bool is_double(void) const;

	/**
	 * 判断本节点数据是否为布尔类型
	 * @return {bool}
	 */
	bool is_bool(void) const;

	/**
	 * 判断本节点数据是否为 null 类型
	 * @return {bool}
	 */
	bool is_null(void) const;

	/**
	 * 判断本节点是否为对象类型
	 * @return {bool}
	 */
	bool is_object(void) const;

	/**
	 * 判断本节点是否为数组类型
	 * @return {bool}
	 */
	bool is_array(void) const;

	/**
	 * 获得该节点类型的描述
	 * @return {const char*}
	 */
	const char* get_type(void) const;

	/**
	 * 当该 json 节点有标签时，本函数用来新的标签值覆盖旧的标签名
	 * @param name {const char*} 新的标签值，为非空字符串
	 * @return {bool} 返回 false 表示该节点没有标签或输入空串，没有进行替换
	 */
	bool set_tag(const char* name);

	/**
	 * 当该 json 节点为叶节点时，本函数用来替换节点的文本值
	 * @param text {const char*} 新的叶节点文本值，为非空字符串
	 * @return {bool} 返回 false 表示该节点非叶节点或输入非法
	 */
	bool set_text(const char* text);

	/**
	 * 将当前 json 节点转换成 json 字符串(包含本 json 节点及其子节点)
	 * @return {const char*}
	 */
	const string& to_string(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 给本 json 节点添加 json_node 子节点对象
	 * @param child {json_node*} 子节点对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时返回子节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_child(json_node* child, bool return_child = false);

	/**
	 * 给本 json 节点添加 json_node 子节点对象
	 * @param child {json_node&} 子节点对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时返回子节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_child(json_node& child, bool return_child = false);

	/**
	 * 创建一个 json 节点对象，并将之添加为本 json 节点的子节点
	 * @param as_array {bool} 是否数组对象
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_array(bool return_child = false);
	json_node& add_child(bool as_array = false, bool return_child = false);

	/**
	 * 创建一个 json 节点对象，并将之添加为本 json 节点的子节点
	 * @param tag {const char*} 标签名
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_child(const char* tag, bool return_child = false);

	/**
	 * 创建一个 json 节点对象，并将之添加为本 json 节点的子节点
	 * @param tag {const char*} 标签名
	 * @param node {json_node*} 标签值指针
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_child(const char* tag, json_node* node,
		bool return_child = false);

	/**
	 * 创建一个 json 节点对象，并将之添加为本 json 节点的子节点
	 * @param tag {const char*} 标签名
	 * @param node {json_node&} 标签值引用
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_child(const char* tag, json_node& node,
		bool return_child = false);

	/**
	 * 创建一个字符串类型的 json 节点对象，并将之添加为本 json 节点的子节点
	 * @param tag {const char*} 标签名
	 * @param value {const char*} 标签值
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 * 注：此处的 add_text 和 add_child 是同样的功能
	 */
	json_node& add_text(const char* tag, const char* value,
		bool return_child = false);

	/**
	 * 创建一个int64 类型的 json 节点对象，并将之添加为本 json 节点的子节点
	 * @param tag {const char*} 标签名
	 * @param value {int64} 标签值
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& add_number(const char* tag, __int64 value,
		bool return_child = false);
#else
	json_node& add_number(const char* tag, long long int value,
		bool return_child = false);
#endif

	/**
	 * 创建一个 double 类型的 json 节点对象，并将之添加为本 json 节点的子节点
	 * @param tag {const char*} 标签名
	 * @param value {double} 标签值
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_double(const char* tag, double value,
		bool return_child = false);

	/**
	 * 创建一个布尔类型的 json 节点对象，并将之添加为本 json 节点的子节点
	 * @param tag {const char*} 标签名
	 * @param value {bool} 标签值
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_bool(const char* tag, bool value,
		bool return_child = false);

	/**
	 * 创建一个 json 字符串对象，并将之添加为本 json 节点的子节点
	 * @param text {const char*} 文本字符串
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_array_text(const char* text,
		bool return_child = false);

	/**
	 * 创建一个 json 数字对象，并将之添加为本 json 节点的子节点
	 * @param value {acl_int64} 数字值
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& add_array_number(__int64 value,
		bool return_child = false);
#else
	json_node& add_array_number(long long int value,
		bool return_child = false);
#endif

	/**
	 * 创建一个 json double 对象，并将之添加为本 json 节点的子节点
	 * @param value {double} 值
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_array_double(double value, bool return_child = false);

	/**
	 * 创建一个 json 布尔对象，并将之添加为本 json 节点的子节点
	 * @param value {bool} 布尔值
	 * @param return_child {bool} 是否需要本函数返回新创建的子节点的引用
	 * @return {json_node&} return_child 为 true 时创建的新节点的引用，
	 *  否则返回本 json 节点对象的引用
	 */
	json_node& add_array_bool(bool value, bool return_child = false);

	/**
	 * @return {json_node&} 返回本节点的父节点引用，在采用级联方式创建 json
	 *  对象时，本函数常被用于返回父节点
	 */
	json_node& get_parent(void) const;

	/////////////////////////////////////////////////////////////////////

	/**
	 * 获得本节点的第一个子节点，需要遍历子节点时必须首先调用此函数
	 * @return {json_node*} 返回空表示没有子节点，返回的非空对象不能
	 *  在外部 delete，因为内部会自动释放
	 */
	json_node* first_child(void);

	/**
	 * 获得本节点的下一个子节点
	 * @return {json_node*} 返回空表示遍历过程结束，返回的非空对象不能
	 *  在外部 delete，因为内部会自动释放
	 */
	json_node* next_child(void);

	const char* operator[] (const char* tag);

	/**
	 * 返回该 json 节点在整个 json 树中的深度
	 * @return {int}
	 */
	int   depth(void) const;

	/**
	 * 返回该 json 节点的下一级子节点的个数
	 * @return {int} 永远 >= 0
	 */
	int   children_count(void) const;

	/**
	 * 将本节点及其子节点从 json 树中删除，其内存将由 json 对象统一释放
	 * @return {int} 被释放的节点数量
	 */
	int detach(void);

	/**
	 * 当在遍历该 json 节点时，内部会动态产生一些临时 json_node 对象，调用
	 * 此函数可以清空这些对象，一旦调用此函数进行了清除，则由 first_child,
	 * next_child 返回的 json_node 节点对象将不再可用，否则会产生内存非法
	 * 访问
	 */
	void clear(void);

	/**
	 * 获得 json 对象的引用
	 * @return {json&}
	 */
	json& get_json(void) const;

	/**
	 * 取出对应于 ACL 库中的 json 节点对象
	 * @return {ACL_JSON_NODE*} 返回节点对象，注：该节点用户不能单独释放
	 */
	ACL_JSON_NODE* get_json_node(void) const;

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
	 * 设置 json 节点
	 * @param node {ACL_JSON_NODE*}
	 */
	void set_json_node(ACL_JSON_NODE* node);

private:
	ACL_JSON_NODE* node_me_;
	json* json_;
	json_node* parent_;
	json_node* parent_saved_;
	std::vector<json_node*>* children_;
	ACL_ITER* iter_;
	string* buf_;
	json_node* obj_;

	union
	{
#if defined(_WIN32) || defined(_WIN64)
		__int64 n;
#else
		long long int n;
#endif
		bool   b;
		double d;
	} node_val_;

	void prepare_iter(void);
};

class ACL_CPP_API json : public pipe_stream, public dbuf_obj
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
	 * 根据一个 json 对象中的一个 json 节点构造一个新的 json 对象
	 * @param node {const json_node&} 源 json 对象中的一个 json 节点
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
	 @return {const char*} 当解析结束后，该返回值表示剩余数据的指针地址
	 */
	const char* update(const char* data);

	/**
	 * 判断是否解析完毕
	 * @return {bool}
	 */
	bool finish(void);

	/**
	 * 重置 json 解析器状态，该 json 对象可以用来对多个 json 数据
	 * 进行解析，在反复使用本 json 解析器前，需要调用本函数重置
	 * 内部 json 解析器状态，清除上一次的解析结果
	 */
	void reset(void);

	/**
	 * 从 json 对象中取得某个标签名的第一个节点
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {json_node*} 返回 json 节点对象，不存在则返回 NULL
	 *  注：返回的 json_node 节点可以修改，但不能删除节点，内部有自动删除机制，
	 *  当调用方法 clear/getElementsByTagName/getElementsByTags 后，节点
	 *  不能再被引用，因为节点的内存被自动释放
	 */
	json_node* getFirstElementByTagName(const char* tag) const;

	/**
	 * 重载运算符，直接获得对应标签名的第一个节点
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {json_node*} 返回 json 节点对象，不存在则返回 NULL
	 *  注：返回的 json_node 节点可以修改，但不能删除节点，内部有自动删除机制，
	 *  当调用方法 clear/getElementsByTagName/getElementsByTags 后，节点
	 *  不能再被引用，因为节点的内存被自动释放
	 */
	json_node* operator[](const char* tag) const;

	/**
	 * 从 json 对象中取得某个标签名的所有节点集合
	 * @param tag {const char*} 标签名(不区分大小写)
	 * @return {const std::vector<json_node*>&} 返回结果集的对象引用，
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的 json_node 节点可以修改，但不能删除节点，内部有自动删除机制，
	 *  当调用方法 clear/getElementsByTagName/getElementsByTags 后，节点
	 *  不能再被引用，因为节点的内存被自动释放
	 */
	const std::vector<json_node*>&
		getElementsByTagName(const char* tag) const;

	/**
	 * 从 json 对象中获得所有的与给定多级标签名相同的 json 节点的集合
	 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，
	 *  如针对 json 数据：
	 *  { 'root': [
	 *      'first': { 'second': { 'third': 'test1' } },
	 *      'first': { 'second': { 'third': 'test2' } },
	 *      'first': { 'second': { 'third': 'test3' } }
	 *    ]
	 *  }
	 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合
	 *  条件的节点
	 * @return {const std::vector<json_node*>&} 符合条件的 json 节点集合, 
	 *  如果查询结果为空，则该集合为空，即：empty() == true
	 *  注：返回的 json_node 节点可以修改，但不能删除节点，内部有自动删除机制，
	 *  当调用方法 clear/getElementsByTagName/getElementsByTags 后，节点
	 *  不能再被引用，因为节点的内存被自动释放
	 */
	const std::vector<json_node*>&
		getElementsByTags(const char* tags) const;

	/**
	 * 从 json 对象中获得所有的与给定多级标签名相同的 json 节点的集合
	 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，
	 *  如针对 json 数据：
	 *  { 'root': [
	 *      'first': { 'second': { 'third': 'test1' } },
	 *      'first': { 'second': { 'third': 'test2' } },
	 *      'first': { 'second': { 'third': 'test3' } }
	 *    ]
	 *  }
	 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合
	 *  条件的节点
	 * @return {json_node*} 返回 NULL 表示不存在
	 */
	json_node* getFirstElementByTags(const char* tags) const;

	/**
	 * 取得 acl 库中的 ACL_JSON 对象
	 * @return {ACL_JSON*} 该值不可能为空，注意用户可以修改该对象的值，
	 *  但不可以释放该对象
	 */
	ACL_JSON* get_json(void) const;

	/////////////////////////////////////////////////////////////////////

	/**
	 * 创建一个 json_node 叶节点对象，该节点对象的格式为：
	 * "tag_name": "tag_value"
	 * @param tag {const char*} 标签名
	 * @param value {const char*} 标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 节点对象
	 */
	json_node& create_node(const char* tag, const char* value);

	/**
	 * 创建一个 json_node 叶节点对象，该节点对象的格式为：
	 * "tag_name": tag_value
	 * @param tag {const char*} 标签名
	 * @param value {int64} 标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 节点对象
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& create_node(const char* tag, __int64 value);
#else
	json_node& create_node(const char* tag, long long int value);
#endif

	/**
	 * 创建一个 json_node 叶节点对象，该节点对象的格式为：
	 * "tag_name": tag_value
	 * @param tag {const char*} 标签名
	 * @param value {double} 标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 节点对象
	 */
	json_node& create_double(const char* tag, double value);

	/**
	 * 创建一个 json_node 叶节点对象，该节点对象的格式为：
	 * "tag_name": true|false
	 * @param tag {const char*} 标签名
	 * @param value {bool} 标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 节点对象
	 */
	json_node& create_node(const char* tag, bool value);

	/**
	 * 创建一个 json_node 叶节点字符串对象，该节点对象的格式为："string"
	 * 按 json 规范，该节点只能加入至数组对象中
	 * @param text {const char*} 文本字符串
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 节点对象
	 */
	json_node& create_array_text(const char* text);

	/**
	 * 创建一个 json_node 叶节点数值对象
	 * 按 json 规范，该节点只能加入至数组对象中
	 * @param value {acl_int64} 数值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 * 不用时调用 reset 来释放这些 json_node 节点对象
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& create_array_number(__int64 value);
#else
	json_node& create_array_number(long long int value);
#endif

	/**
	 * 创建一个 json_node 叶节点数值对象
	 * 按 json 规范，该节点只能加入至数组对象中
	 * @param value {double} 值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 * 不用时调用 reset 来释放这些 json_node 节点对象
	 */
	json_node& create_array_double(double value);

	/**
	 * 创建一个 json_node 叶节点布尔对象
	 * 按 json 规范，该节点只能加入至数组对象中
	 * @param value {bool} 布尔值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 * 不用时调用 reset 来释放这些 json_node 节点对象
	 */
	json_node& create_array_bool(bool value);

	/**
	 * 创建一个 json_node 节点容器对象，该对象没有标签,
	 * 该节点对象的格式为："{}" 或数组对象 "[]"
	 * @param as_array {bool} 是否数组对象
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 节点对象
	 */
	json_node& create_node(bool as_array = false);
	json_node& create_array(void);

	/**
	 * 创建一个 json_node 节点对象，该节点对象的格式为：tag_name: {}
	 * 或 tag_name: []
	 * @param tag {const char*} 标签名
	 * @param node {json_node*} json 节点对象作为标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 节点对象
	 */
	json_node& create_node(const char* tag, json_node* node);

	/**
	 * 创建一个 json_node 节点对象，该节点对象的格式为：tag_name: {}
	 * 或 tag_name: []
	 * @param tag {const char*} 标签名
	 * @param node {json_node&} json 节点对象作为标签值
	 * @return {json_node&} 新产生的 json_node 对象不需要用户手工释放，
	 *  因为在 json 对象被释放时这些节点会自动被释放，当然用户也可以在
	 *  不用时调用 reset 来释放这些 json_node 节点对象
	 */
	json_node& create_node(const char* tag, json_node& node);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 将一个 json 对象中的一个 json 节点复制至任一 json 对象中的一个
	 * 新 json 节点中并返回该新的 json 节点
	 * @param node {json_node*} 源 json 对象的一个 json 节点
	 * @return {json_node&} 当前目标 json 对象中新创建的 json 节点
	 */
	json_node& duplicate_node(const json_node* node);

	/**
	 * 将一个 json 对象中的一个 json 节点复制至任一 json 对象中的一个
	 * 新 json 节点中并返回该新的 json 节点
	 * @param node {json_node&} 源 json 对象的一个 json 节点
	 * @return {json_node&} 当前目标 json 对象中新创建的 json 节点
	 */
	json_node& duplicate_node(const json_node& node);

	/**
	 * 获得根节点对象
	 * @return {json_node&}
	 */
	json_node& get_root(void);

	/**
	 * 开始遍历该 json 对象并获得第一个节点
	 * @return {json_node*} 返回空表示该 json 对象为空节点
	 *  注：返回的节点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	json_node* first_node(void);

	/**
	 * 遍历该 json 对象的下一个 json 节点
	 * @return {json_node*} 返回空表示遍历完毕
	 *  注：返回的节点对象用户不能手工释放，因为该对象被
	 *  内部库自动释放
	 */
	json_node* next_node(void);

	/**
	 * 将 json 对象树转成字符串
	 * @param out {string&} 存储转换结果的缓冲区
	 */
	void build_json(string& out) const;

	/**
	 * 将 json 对象树转换成 json 字符串
	 * @return {const string&}
	 */
	const string& to_string(void);

	// pipe_stream 虚函数重载

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear(void);

private:
	// 对应于 acl 库中的 ACL_JSON 对象
	ACL_JSON *json_;
	// json 对象树中的根节点对象
	json_node* root_;
	// 临时的 json 节点查询结果集
	std::vector<json_node*> nodes_query_;
	// 由该 json 容器分配的 json 节点集合
	std::list<json_node*> nodes_tmp_;
	// 缓冲区
	string* buf_;
	ACL_ITER* iter_;
};

} // namespace acl
