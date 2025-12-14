#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include <vector>
#include "dbuf_pool.hpp"
#include "pipe_stream.hpp"

struct ACL_JSON_NODE;
struct ACL_JSON;
struct ACL_ITER;

/**
 * This is a C++ wrapper for ACL library's json parsing library. If you don't care too much about performance,
 * you can directly use this class. If you are very concerned about performance, you can directly use ACL library's
 * json parsing library. Because this class is also a wrapper for ACL library's json parsing process, and there are multiple
 * wrapper processes, which may slightly affect some performance. However, this impact is very small.
 * Additionally, json object and attached json node objects are managed by internal dbuf pool, so when json object
 * is destroyed, all json node objects will be automatically released.
 */

namespace acl {

class string;
class json;

/**
 * json node, can only be created through json.create_node() method.
 */
class ACL_CPP_API json_node : public dbuf_obj {
public:
	/**
	 * Get tag name of this json node.
	 * @return {const char*} Json node tag name. Returns empty to indicate
	 *  tag does not exist. You need to check return value.
	 */
	const char* tag_name() const;

	/**
	 * Get text tag value of this json node. When value is numeric type or boolean type, internally
	 * automatically converts to string.
	 * @return {const char*} Returns empty to indicate no text tag value.
	 */
	const char* get_text() const;

	/**
	 * When json node has child nodes, returns json child node corresponding to this json node's tag.
	 * @return {const json_node*} Returns NULL to indicate no child nodes.
	 *  Note: get_text and get_obj cannot both return NULL.
	 */
	json_node* get_obj() const;

	/**
	 * When json node is string type, this function returns string value.
	 * @return {const char*} Returns NULL to indicate this node is not string type.
	 */
	const char* get_string() const;

	/**
	 * When json node is integer type, this function returns integer value pointer.
	 * @return {const long long int*} Returns NULL when this object is not integer type.
	 */
#if defined(_WIN32) || defined(_WIN64)
	const __int64* get_int64(void) const;
#else
	const long long int* get_int64() const;
#endif

	/**
	 * When json node is floating point type, this function returns floating point value pointer.
	 * @return {const double*} Returns NULL when this object is not floating point type.
	 */
	const double *get_double() const;

	/**
	 * When json node is boolean type, this function returns boolean value pointer.
	 * @return {bool*} Returns NULL when this object is not boolean type.
	 */
	const bool* get_bool() const;

	/**
	 * Determine whether this node's content is string type.
	 * @return {bool}
	 */
	bool is_string() const;

	/**
	 * Determine whether this node's content is integer type.
	 * @return {bool}
	 */
	bool is_number() const;

	/**
	 * Determine whether this node's content is floating point type.
	 * @return {bool}
	 */
	bool is_double() const;

	/**
	 * Determine whether this node's content is boolean type.
	 * @return {bool}
	 */
	bool is_bool() const;

	/**
	 * Determine whether this node's content is null type.
	 * @return {bool}
	 */
	bool is_null() const;

	/**
	 * Determine whether this node is object type.
	 * @return {bool}
	 */
	bool is_object() const;

	/**
	 * Determine whether this node is array type.
	 * @return {bool}
	 */
	bool is_array() const;

	/**
	 * Get string representation of this node's type.
	 * @return {const char*}
	 */
	const char* get_type() const;

	/**
	 * When json node has tag, this function replaces old tag name with new tag value.
	 * @param name {const char*} New tag value, cannot be empty string.
	 * @return {bool} Returns false to indicate this node has no tag, or empty string was passed, or replacement failed.
	 */
	bool set_tag(const char* name);

	/**
	 * When json node is leaf node, this function replaces node's text value.
	 * @param text {const char*} New leaf node's text value, cannot be empty string.
	 * @return {bool} Returns false to indicate this node is not a leaf node or error occurred.
	 */
	bool set_text(const char* text);

	/**
	 * Convert current json node to json string (including json node and its child nodes).
	 * @param out {string*} When not empty, uses this buffer. Otherwise uses internal buffer.
	 * @return {const char*}
	 */
	const string& to_string(string* out = NULL) const;

	/////////////////////////////////////////////////////////////////////

	/**
	 * Add json_node child node object to json node.
	 * @param child {json_node*} Child node object.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns child node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_child(json_node* child, bool return_child = false);

	/**
	 * Add json_node child node object to json node.
	 * @param child {json_node&} Child node object.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns child node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_child(json_node& child, bool return_child = false);

	/**
	 * Create a json node object, and add it as this json node's child node.
	 * @param as_array {bool} Whether it is array.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_child(bool as_array = false, bool return_child = false);
	json_node& add_array(bool return_child = false);

	/**
	 * Create a json node object, and add it as this json node's child node.
	 * @param tag {const char*} Tag name.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_child(const char* tag, bool return_child = false);

	/**
	 * Create a json node object, and add it as this json node's child node.
	 * @param tag {const char*} Tag name.
	 * @param node {json_node*} Tag value pointer.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_child(const char* tag, json_node* node,
		bool return_child = false);

	/**
	 * Create a json node object, and add it as this json node's child node.
	 * @param tag {const char*} Tag name.
	 * @param node {json_node&} Tag value reference.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_child(const char* tag, json_node& node,
		bool return_child = false);

	/**
	 * Create a string type json node object, and add it as this json node's child node.
	 * @param tag {const char*} Tag name.
	 * @param value {const char*} Tag value.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 * Note: Here add_text and add_child have the same function.
	 */
	json_node& add_text(const char* tag, const char* value,
		bool return_child = false);

	/**
	 * Create an int64 type json node object, and add it as this json node's child node.
	 * @param tag {const char*} Tag name.
	 * @param value {int64} Tag value.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& add_number(const char* tag, __int64 value,
		bool return_child = false);
#else
	json_node& add_number(const char* tag, long long int value,
		bool return_child = false);
#endif

	/**
	 * Create a double type json node object, and add it as this json node's child node.
	 * @param tag {const char*} Tag name.
	 * @param value {double} Tag value.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_double(const char* tag, double value,
		bool return_child = false);

	/**
	 * Create a double type json node object, and add it as this json node's child node.
	 * @param tag {const char*} Tag name.
	 * @param value {double} Tag value.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @param precision {int} Decimal precision. Effective when > 0. Default value is 4.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_double(const char* tag, double value, int precision,
		bool return_child = false);

	/**
	 * Create a boolean type json node object, and add it as this json node's child node.
	 * @param tag {const char*} Tag name.
	 * @param value {bool} Tag value.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_bool(const char* tag, bool value,
		bool return_child = false);

	/**
	 * Create a null type json node object, and add it as this json node's child node.
	 * @param tag {const char*} Tag name.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_null(const char* tag, bool return_child = false);

	/**
	 * Create a json string object, and add it as this json node's child node.
	 * @param text {const char*} Text string.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_array_text(const char* text, bool return_child = false);

	/**
	 * Create a json numeric object, and add it as this json node's child node.
	 * @param value {acl_int64} Numeric value.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& add_array_number(__int64 value,
		bool return_child = false);
#else
	json_node& add_array_number(long long int value, bool return_child = false);
#endif

	/**
	 * Create a json double object, and add it as this json node's child node.
	 * @param value {double} Value.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_array_double(double value, bool return_child = false);

	/**
	 * Create a json boolean object, and add it as this json node's child node.
	 * @param value {bool} Boolean value.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_array_bool(bool value, bool return_child = false);

	/**
	 * Create a json null object, and add it as this json node's child node.
	 * @param return_child {bool} Whether to return newly created child node object.
	 * @return {json_node&} When return_child is true, returns newly created node reference.
	 *  Otherwise returns this json node reference.
	 */
	json_node& add_array_null(bool return_child = false);

	/**
	 * @return {json_node&} Returns parent node reference of this node. Internally uses reference counting method. When json
	 *  object is destroyed, reference counting automatically releases parent node.
	 */
	json_node& get_parent() const;

	/////////////////////////////////////////////////////////////////////

	/**
	 * Get first child node of this node. When traversing child nodes, you need to call this function first.
	 * @return {json_node*} Returns empty to indicate no child nodes. Returned non-empty object
	 *  should not be deleted externally, because it is automatically released internally.
	 */
	json_node* first_child();

	/**
	 * Get next child node of this node.
	 * @return {json_node*} Returns empty to indicate traversal finished. Returned non-empty object
	 *  should not be deleted externally, because it is automatically released internally.
	 */
	json_node* next_child();

	/**
	 * In child node collection, remove and release specified child node, and return next child node.
	 * @param child {json_node*} Current json node's child node returned by first_child/next_child/remove_child
	 *  during traversal. This node will be removed from Json tree and released.
	 * @return {json_node*} Next child node of current node.
	 */
	json_node* free_child(json_node* child);

	/**
	 * Get json child node with corresponding tag name from current json node's child node collection.
	 * @param tag {const char*} Json child node's tag name.
	 * @return {json_node*} Returns NULL to indicate not found.
	 */
	json_node* operator[] (const char* tag);

	/**
	 * Get depth of this json node in json tree.
	 * @return {int}
	 */
	int   depth() const;

	/**
	 * Get number of child nodes of this json node.
	 * @return {int} Always >= 0.
	 */
	int   children_count() const;

	/**
	 * Detach this node and its child nodes from json tree. Memory will be released by json object uniformly.
	 * @return {int} Number of detached nodes.
	 */
	int detach();

	/**
	 * Disable current json node. When building json string, disabled nodes will be ignored. This node is not
	 * removed from json node tree, so when traversing next time, you can still use this node.
	 * @param yes {bool} Whether to disable json node. false means enable, true means disable.
	 */
	void disable(bool yes);

	/**
	 * Determine whether current json node is disabled.
	 * @return {bool}
	 */
	bool disabled() const;

	/**
	 * When traversing json nodes, internally dynamically creates some temporary json_node objects. When
	 * traversal finishes, you need to call this function to clear these objects. Otherwise,
	 * json_node node objects returned by first_child,
	 * next_child will no longer be available, which may cause memory leak.
	 */
	void clear();

	/**
	 * Get json object reference.
	 * @return {json&}
	 */
	json& get_json() const;

	/**
	 * Get corresponding json node object in ACL library.
	 * @return {ACL_JSON_NODE*} Returns node object. Note: This node object should not be manually released by users.
	 */
	ACL_JSON_NODE* get_json_node() const;

private:
	friend class json;
	friend class dbuf_guard;

	/**
	 * Constructor. This object can only be created by json object.
	 * @param node {ACL_JSON_NODE*} ACL_JSON_NODE structure object in ACL library.
	 */
	json_node(ACL_JSON_NODE* node, json* json_ptr);

	/**
	 * This object needs to be dynamically allocated.
	 */
	~json_node();

	/**
	 * Set json node.
	 * @param node {ACL_JSON_NODE*}
	 */
	void set_json_node(ACL_JSON_NODE* node);

private:
	ACL_JSON_NODE* node_me_;
	json* json_;
	dbuf_guard* dbuf_;
	json_node* parent_;
	ACL_ITER* iter_;
	string* buf_;
	json_node* obj_;

	union {
#if defined(_WIN32) || defined(_WIN64)
		__int64 n;
#else
		long long int n;
#endif
		bool   b;
		double d;
	} node_val_;
};

class ACL_CPP_API json : public pipe_stream, public dbuf_obj {
public:
	/**
	 * Constructor, used for parsing json string data into json object.
	 * @param data {const char*} Json format string data. This parameter can be
	 *  complete json string, can also be partial json string, can also be empty pointer.
	 *  Regardless of which case, users can still call update function to parse partial or complete json string data. In fact, constructor
	 *  also calls update internally when data is not empty.
	 * @param dbuf {dbuf_guard*} When not empty, uses as memory pool object. Otherwise, internally
	 *  automatically creates a memory pool object.
	 */
	json(const char* data = NULL, dbuf_guard* dbuf = NULL);

	/**
	 * Construct a new json object from a json node in a json object.
	 * @param node {const json_node&} A json node in source json object.
	 * @param dbuf {dbuf_guard*} When not empty, uses as memory pool object. Otherwise, internally
	 * automatically creates a memory pool object.
	 */
	json(const json_node& node, dbuf_guard* dbuf = NULL);

	~json();

	/**
	 * Set whether to automatically parse word boundaries when parsing.
	 * @param on {bool}
	 * @return {json&}
	 */
	json& part_word(bool on);

	/**
	 * Streaming format: call this function in a loop to parse json data. You can also parse
	 * complete json data at once. When reusing this json object, you should call reset() function before
	 * parsing next json data to clear last parse result.
	 * @param data {const char*} json data.
	 @return {const char*} When parsing is not complete, return value indicates pointer to remaining data.
	 */
	const char* update(const char* data);

	/**
	 * Determine whether parsing is complete.
	 * @return {bool}
	 */
	bool finish();

	/**
	 * Reset json object state. Json object internally automatically manages json tree
	 * memory. Before reusing this json object, you need to call this function to
	 * reset internal json object state and clear last parse result.
	 */
	void reset();

	/**
	 * Get first node with a certain tag name from json tree.
	 * @param tag {const char*} Tag name (case-insensitive).
	 * @return {json_node*} Returns json node object. Returns NULL when not found.
	 *  Note: Returned json_node node data can be modified, but do not delete node. Internally automatically manages deletion.
	 *  After calling clear/getElementsByTagName/getElementsByTags, node
	 *  will no longer be available, because node memory is automatically released.
	 */
	json_node* getFirstElementByTagName(const char* tag) const;

	/**
	 * Directly get first node with corresponding tag name from root node.
	 * @param tag {const char*} Tag name (case-insensitive).
	 * @return {json_node*} Returns json node object. Returns NULL when not found.
	 *  Note: Returned json_node node data can be modified, but do not delete node. Internally automatically manages deletion.
	 *  After calling clear/getElementsByTagName/getElementsByTags, node
	 *  will no longer be available, because node memory is automatically released.
	 */
	json_node* operator[](const char* tag) const;

	/**
	 * Get node collection of all nodes with a certain tag name from json tree.
	 * @param tag {const char*} Tag name (case-insensitive).
	 * @return {const std::vector<json_node*>&} Returns reference to result object.
	 *  When query result is empty, collection is empty, i.e., empty() == true.
	 *  Note: Returned json_node node data can be modified, but do not delete node. Internally automatically manages deletion.
	 *  After calling clear/getElementsByTagName/getElementsByTags, node
	 *  will no longer be available, because node memory is automatically released.
	 */
	const std::vector<json_node*>& getElementsByTagName(const char* tag) const;

	/**
	 * Get collection of all json nodes with same hierarchical tag name from json tree.
	 * @param tags {const char*} Hierarchical tag name, separated by '/'. For example, for json data:
	 *  { 'root': [
	 *      'first': { 'second': { 'third': 'test1' } },
	 *      'first': { 'second': { 'third': 'test2' } },
	 *      'first': { 'second': { 'third': 'test3' } }
	 *    ]
	 *  }
	 *  You can use hierarchical tag name root/first/second/third to query all matching
	 *  nodes at once.
	 * @return {const std::vector<json_node*>&} Returns json node collection, 
	 *  When query result is empty, collection is empty, i.e., empty() == true.
	 *  Note: Returned json_node node data can be modified, but do not delete node. Internally automatically manages deletion.
	 *  After calling clear/getElementsByTagName/getElementsByTags, node
	 *  will no longer be available, because node memory is automatically released.
	 */
	const std::vector<json_node*>& getElementsByTags(const char* tags) const;

	/**
	 * Get collection of all json nodes with same hierarchical tag name from json tree.
	 * @param tags {const char*} Hierarchical tag name, separated by '/'. For example, for json data:
	 *  { 'root': [
	 *      'first': { 'second': { 'third': 'test1' } },
	 *      'first': { 'second': { 'third': 'test2' } },
	 *      'first': { 'second': { 'third': 'test3' } }
	 *    ]
	 *  }
	 *  You can use hierarchical tag name root/first/second/third to query all matching
	 *  nodes at once.
	 * @return {json_node*} Returns NULL to indicate not found.
	 */
	json_node* getFirstElementByTags(const char* tags) const;

	/**
	 * Get ACL_JSON object in acl library.
	 * @return {ACL_JSON*} Return value may be empty. Note: Users should not modify this object's value or
	 *  release this object.
	 */
	ACL_JSON* get_json() const;

	/////////////////////////////////////////////////////////////////////

	/**
	 * Create a json_node leaf node object. This node's format is:
	 * "tag_name": "tag_value"
	 * @param tag {const char*} Tag name.
	 * @param value {const char*} Tag value.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 *  call reset to release these json_node node objects when not needed.
	 */
	json_node& create_node(const char* tag, const char* value);

	/**
	 * Create a json_node leaf node object. This node's format is:
	 * "tag_name": tag_value
	 * @param tag {const char*} Tag name.
	 * @param value {int64} Tag value.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 *  call reset to release these json_node node objects when not needed.
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& create_node(const char* tag, __int64 value);
#else
	json_node& create_node(const char* tag, long long int value);
#endif

	/**
	 * Create a json_node leaf node object. This node's format is:
	 * "tag_name": tag_value
	 * @param tag {const char*} Tag name.
	 * @param value {double} Tag value.
	 * @param precision {int} Precision of decimal point. Effective when > 0. Default is 4.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 *  call reset to release these json_node node objects when not needed.
	 */
	json_node& create_double(const char* tag, double value, int precision = 4);

	/**
	 * Create a json_node leaf node object. This node's format is:
	 * "tag_name": true|false
	 * @param tag {const char*} Tag name.
	 * @param value {bool} Tag value.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 *  call reset to release these json_node node objects when not needed.
	 */
	json_node& create_node(const char* tag, bool value);

	/**
	 * Create a json_node null leaf node object. This node's format is:
	 * "tag_name": null
	 * @param tag {const char*} Tag name.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 *  call reset to release these json_node node objects when not needed.
	 */
	json_node& create_null(const char* tag);

	/**
	 * Create a json_node leaf node string object. This node's format is: "string"
	 * According to json specification, this node can only be added to array.
	 * @param text {const char*} Text string.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 *  call reset to release these json_node node objects when not needed.
	 */
	json_node& create_array_text(const char* text);

	/**
	 * Create a json_node leaf node numeric object.
	 * According to json specification, this node can only be added to array.
	 * @param value {acl_int64} Numeric value.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 * call reset to release these json_node node objects when not needed.
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& create_array_number(__int64 value);
#else
	json_node& create_array_number(long long int value);
#endif

	/**
	 * Create a json_node leaf node numeric object.
	 * According to json specification, this node can only be added to array.
	 * @param value {double} Value.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 * call reset to release these json_node node objects when not needed.
	 */
	json_node& create_array_double(double value);

	/**
	 * Create a json_node leaf node boolean object.
	 * According to json specification, this node can only be added to array.
	 * @param value {bool} Boolean value.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 * call reset to release these json_node node objects when not needed.
	 */
	json_node& create_array_bool(bool value);

	/**
	 * Create a json_node leaf node null object.
	 * According to json specification, this node can only be added to array.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 * call reset to release these json_node node objects when not needed.
	 */
	json_node& create_array_null();

	/**
	 * Create a json_node node object. This object has no tag,
	 * this node's format is: "{}" (object) or "[]" (array)
	 * @param as_array {bool} Whether it is array.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 *  call reset to release these json_node node objects when not needed.
	 */
	json_node& create_node(bool as_array = false);
	json_node& create_array();

	/**
	 * Create a json_node node object. This node's format is: tag_name: {}
	 * or tag_name: []
	 * @param tag {const char*} Tag name.
	 * @param node {json_node*} Json node object as tag value.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 *  call reset to release these json_node node objects when not needed.
	 */
	json_node& create_node(const char* tag, json_node* node);

	/**
	 * Create a json_node node object. This node's format is: tag_name: {}
	 * or tag_name: []
	 * @param tag {const char*} Tag name.
	 * @param node {json_node&} Json node object as tag value.
	 * @return {json_node&} Newly created json_node object. Users do not need to manually release it,
	 *  because when json object is destroyed, these nodes will be automatically released. However, users can also
	 *  call reset to release these json_node node objects when not needed.
	 */
	json_node& create_node(const char* tag, json_node& node);

	/**
	 * Remove specified json node from json tree, simultaneously release occupied memory resources. Applications
	 * should no longer use this json node object.
	 * @param node {json_node*} This json node and all its child nodes will be removed from json
	 *  tree, and occupied memory resources will be released when json object is destroyed by dbuf pool.
	 */
	void remove(json_node* node);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Copy a json node from one json object to another json object's json node, and insert copied new json node.
	 * @param node {json_node*} A json node in source json object.
	 * @return {json_node&} Newly created json node in current target json object.
	 */
	json_node& duplicate_node(const json_node* node);

	/**
	 * Copy a json node from one json object to another json object's json node, and insert copied new json node.
	 * @param node {json_node&} A json node in source json object.
	 * @return {json_node&} Newly created json node in current target json object.
	 */
	json_node& duplicate_node(const json_node& node);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Get root node object.
	 * @return {json_node&}
	 */
	json_node& get_root();

	/**
	 * Start traversing json object and get first node.
	 * @return {json_node*} Returns empty to indicate json object is empty node.
	 *  Note: Returned node object does not need to be manually released, because this object
	 *  internally automatically releases it.
	 */
	json_node* first_node();

	/**
	 * Get next json node in json tree.
	 * @return {json_node*} Returns empty to indicate traversal finished.
	 *  Note: Returned node object does not need to be manually released, because this object
	 *  internally automatically releases it.
	 */
	json_node* next_node();

	/**
	 * In traversal, remove and release current traversed json node, and return next json node.
	 * @param curr {json_node*} Node returned by first_node/next_node/free_node.
	 * @return {json_node*} Next json node.
	 */
	json_node* free_node(json_node* curr);

	/**
	 * Convert json tree to string.
	 * @param out {string&} Buffer to store converted result.
	 * @param add_space {bool} Whether to automatically add spaces between separators when building json.
	 */
	void build_json(string& out, bool add_space = false) const;

	/**
	 * Convert json object to json string.
	 * @param out {string*} When not empty, uses this buffer. Otherwise uses internal buffer.
	 * @param add_space {bool} Whether to automatically add spaces between separators when building json.
	 * @return {const string&}
	 */
	const string& to_string(string* out = NULL, bool add_space = false) const;

	/**
	 * Get memory pool object pointer.
	 * @return {dbuf_guard*}
	 */
	dbuf_guard* get_dbuf() const {
		return dbuf_;
	}

	// pipe_stream virtual function implementation.

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear();

private:
	// Memory pool object, suitable for managing small memory.
	dbuf_guard* dbuf_;
	dbuf_guard* dbuf_internal_;

	// Corresponding ACL_JSON object in acl library.
	ACL_JSON *json_;
	// Root node object in json object tree.
	json_node* root_;
	// Temporary json node query result.
	std::vector<json_node*> nodes_query_;
	// Buffer.
	string* buf_;
	ACL_ITER* iter_;
};

} // namespace acl

