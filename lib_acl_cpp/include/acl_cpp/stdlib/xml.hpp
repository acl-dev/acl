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
 * This is a C++ wrapper for ACL library's XML parsing library. If you don't
 * care too much about performance,
 * you can directly use this class. If you are very concerned about performance,
 * you can directly use ACL library's
 * XML parsing library. Because this class is also a wrapper for ACL library's
 * XML parsing process, and there are multiple
 * wrapper processes, which may slightly affect some performance. However, this
 * impact is very small.
 */

namespace acl {

class xml;
class xml_node;
class istream;

class ACL_CPP_API xml_attr : public dbuf_obj {
public:
	/**
	 * Get attribute name.
	 * @return {const char*} Attribute name. This function will never return empty
	 * pointer, but return value
	 *  may be "\0".
	 */
	virtual const char* get_name() const = 0;

	/**
	 * Get attribute value.
	 * @return {const char*} Attribute value. This function will never return empty
	 * pointer, but return value
	 *  may be "\0".
	 */
	virtual const char* get_value() const = 0;

protected:
	friend class xml_node;

	xml_attr(xml_node* node) : node_(node) {}
	virtual ~xml_attr() {}

	xml_node* node_;
};

class ACL_CPP_API xml_node : public dbuf_obj
{
public:
	/**
	 * Get tag name of this XML node.
	 * @return {const char*} XML node tag name. Returns empty to indicate
	 *  tag does not exist (e.g., tag xxxx), you need to check return value.
	 */
	virtual const char* tag_name() const = 0;

	/**
	 * Get ID attribute of this XML node. If attribute does not exist, returns
	 * empty pointer.
	 * @return {const char*} When ID attribute exists, returns corresponding value.
	 * Otherwise returns empty.
	 */
	virtual const char* id() const = 0;

	/**
	 * Get text content of this XML node.
	 * @return {const char*} Returns empty to indicate no text content.
	 */
	virtual const char* text() const = 0;

	/**
	 * Get a certain attribute value of this XML node.
	 * @param name {const char*} Attribute name.
	 * @return {const char*} Attribute value. Returns empty to indicate attribute
	 * does not exist.
	 */
	virtual const char* attr_value(const char* name) const = 0;

	/**
	 * Get a certain attribute value of this XML node by subscript.
	 * @param name {const char*} Attribute name.
	 * @return {const char*} Attribute value. Returns empty to indicate attribute
	 * does not exist.
	 */
	const char* operator[](const char* name) const;

	/**
	 * When traversing node attributes, you need to call this function to get first
	 * attribute object.
	 * @return {const xml_attr*} Returns first attribute object. Returns empty to
	 * indicate
	 *  this node has no attributes.
	 */
	virtual const xml_attr* first_attr() const = 0;

	/**
	 * When traversing node attributes, call this function to get next attribute
	 * object.
	 * @return {const xml_attr*} Returns next attribute object. Returns empty to
	 * indicate
	 *  traversal finished.
	 */
	virtual const xml_attr* next_attr() const = 0;

	/**
	 * Add attribute to XML node.
	 * @param name {const char*} Attribute name.
	 * @param value {const char*} Attribute value.
	 * @return {xml_node&}
	 */
	virtual xml_node& add_attr(const char* name, const char* value) = 0;

	/**
	 * Add attribute to XML node.
	 * @param name {const char*} Attribute name.
	 * @param n {char} Attribute value.
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, char n);

	/**
	 * Add attribute to XML node.
	 * @param name {const char*} Attribute name.
	 * @param n {int} Attribute value.
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, int n);

	/**
	 * Add attribute to XML node.
	 * @param name {const char*} Attribute name.
	 * @param n {size_t} Attribute value.
	 * @return {xml_node&}
	 */
	xml_node& add_attr(const char* name, size_t n);

	/**
	 * Add attribute to XML node.
	 * @param name {const char*} Attribute name.
	 * @param n {acl_int64} Attribute value.
	 * @return {xml_node&}
	 */
#if defined(_WIN32) || defined(_WIN64)
	xml_node& add_attr(const char* name, __int64 n);
#else
	xml_node& add_attr(const char* name, long long int n);
#endif

	/**
	 * Set text content of xml node.
	 * @param str {const char*} String content.
	 * @param append {bool} When setting text, whether it is append mode or
	 * overwrite mode. When it is append mode,
	 * if this node originally has text content, new content will be appended to
	 * original text content. Otherwise, overwrite.
	 * @return {xml_node&}
	 */
	virtual xml_node& set_text(const char* str, bool append = false) = 0;

	/**
	 * Set xml node, and simultaneously use data in input stream as this node's
	 * text content.
	 * @param in {istream&} Input stream object.
	 * @param off {size_t} For file stream, specifies start position of data to be
	 * read.
	 * @param len {size_t} Number of bytes to be read. When this value is 0, reads
	 * until stream ends.
	 * @return {xml_node&}
	 */
	virtual xml_node& set_text(istream& in, size_t off = 0,
		size_t len = 0) = 0;

	/**
	 * Set text content of xml node.
	 * @param number {long long int} 64-bit integer.
	 * @return {xml_node&}
	 */
#if defined(_WIN32) || defined(_WIN64)
	xml_node& set_text(__int64 number);
#else
	xml_node& set_text(long long int number);
#endif

	/**
	 * Add xml_node child node object to xml node.
	 * @param child {xml_node*} Child node object.
	 * @param return_child {bool} Whether to return newly created child node
	 * object.
	 * @return {xml_node&} When return_child is true, returns child node reference.
	 *  Otherwise returns this xml node reference.
	 */
	virtual xml_node& add_child(xml_node* child,
		bool return_child = false) = 0;
	
	/**
	 * Add xml_node child node object to xml node.
	 * @param child {xml_node&} Child node object.
	 * @param return_child {bool} Whether to return newly created child node
	 * object.
	 * @return {xml_node&} When return_child is true, returns child node reference.
	 *  Otherwise returns this xml node reference.
	 */
	xml_node& add_child(xml_node& child, bool return_child = false);

	/**
	 * Add xml_node child node object to xml node.
	 * @param tag {const char* tag} Child node object's tag name.
	 * @param return_child {bool} Whether to return newly created child node
	 * object.
	 * @param str {const char*} Text string.
	 * @return {xml_node&} When return_child is true, returns child node reference.
	 *  Otherwise returns this xml node reference.
	 */
	xml_node& add_child(const char* tag, bool return_child = false,
		const char* str = NULL);

	/**
	 * Add xml_node child node object to xml node.
	 * @param tag {const char* tag} Child node object's tag name.
	 * @param txt {long long int} Text content in node, non-empty string.
	 * @param return_child {bool} Whether to return newly created child node
	 * object.
	 * @return {xml_node&} When return_child is true, returns child node reference.
	 *  Otherwise returns this xml node reference.
	 */
	xml_node& add_child(const char* tag, const char* txt,
		bool return_child = false);

	/**
	 * Add xml_node child node object to xml node.
	 * @param tag {const char* tag} Child node object's tag name.
	 * @param number {long long int} 64-bit integer.
	 * @param return_child {bool} Whether to return newly created child node
	 * object.
	 * @return {xml_node&} When return_child is true, returns child node reference.
	 *  Otherwise returns this xml node reference.
	 */
#if defined(_WIN32) || defined(_WIN64)
	xml_node& add_child(const char* tag, __int64 number,
		bool return_child = false);
#else
	xml_node& add_child(const char* tag, long long int number,
		bool return_child = false);
#endif

	/**
	 * Add xml_node child node object to xml node and simultaneously use data in
	 * input stream as node's text.
	 * @param tag {const char* tag} Child node object's tag name.
	 * @param in {istream&} Input stream object.
	 * @param off {size_t} For file stream, specifies start position of data to be
	 * read.
	 * @param len {size_t} Number of bytes to be read. When this value is 0, reads
	 * until stream ends.
	 * @param return_child {bool} Whether to return newly created child node
	 * object.
	 * @return {xml_node&} When return_child is true, returns child node reference.
	 *  Otherwise returns this xml node reference.
	 */
	xml_node& add_child(const char* tag, istream& in,
		size_t off = 0, size_t len = 0, bool return_child = false);

	/**
	 * Get parent node of this node.
	 * @return {xml_node&}
	 */
	virtual xml_node& get_parent() const = 0;

	/**
	 * Set parent node of this node.
	 * @param node {xml_node*} Parent node.
	 * @return {xml_node&}
	 */
	virtual xml_node& set_parent(xml_node* node) = 0;

	/**
	 * Detach this node and its child nodes from xml tree. Memory will be released
	 * by xml object uniformly.
	 * @return {int} Returns number of detached nodes.
	 */
	virtual int detach() = 0;

	/**
	 * Get first child node of this node. When traversing child nodes, you need to
	 * call this function first.
	 * @return {xml_node*} Returns empty to indicate no child nodes.
	 */
	virtual xml_node* first_child() = 0;

	/**
	 * Get next child node of this node.
	 * @return {xml_node*} Returns empty to indicate traversal finished.
	 */
	virtual xml_node* next_child() = 0;

	/**
	 * Get depth of this XML node in XML tree.
	 * @return {int}
	 */
	virtual int depth() const = 0;

	/**
	 * Determine whether current node is root node in xml tree.
	 * @return {bool}
	 */
	virtual bool is_root() const = 0;

	/**
	 * Get number of child nodes of this xml node.
	 * @return {int} Always >= 0.
	 */
	virtual int children_count() const = 0;

	/**
	 * When traversing xml nodes, internally dynamically creates some temporary
	 * xml_node objects. When
	 * traversal finishes, you need to call this function to clear these objects.
	 * Otherwise,
	 * xml_node node objects returned by first_child/next_child will no longer be
	 * available, which
	 * may cause memory leak.
	 */
	void clear();

	/**
	 * Get xml object reference.
	 * @return {xml&}
	 */
	xml& get_xml() const;

protected:
	friend class xml;
	friend class dbuf_guard;

	/**
	 * xml node constructor.
	 * @param xml_ptr {xml*} xml object pointer, non-empty.
	 */
	xml_node(xml* xml_ptr);

	/**
	 * This object needs to be dynamically allocated.
	 */
	virtual ~xml_node();

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
	 * @param dbuf_nblock {size_t} Initial block count of internally created
	 * dbuf_guard.
	 * @param dbuf_capacity {size_t} Initial capacity of internally created
	 * dbuf_guard.
	 */
	xml(size_t dbuf_nblock = 2, size_t dbuf_capacity = 100);
	virtual ~xml();

	/**
	 * For non-closed tags, whether to automatically add closing character '/'.
	 * Default is not to add.
	 * @param on {bool}
	 * @return {xml&}
	 */
	virtual xml& ignore_slash(bool on) = 0;

	/**
	 * When parsing xml data, whether to automatically decode xml encoding. Default
	 * is to decode.
	 * @param on {bool}
	 * @return {xml&}
	 */
	virtual xml& xml_decode(bool on) = 0;

	/**
	 * When building xml data, whether to automatically encode xml encoding.
	 * Default is to encode.
	 * @param on {bool}
	 * @return {xml&}
	 */
	virtual xml& xml_encode(bool on) = 0;

	/**
	 * When parsing xml, whether to allow multiple root nodes (internal default is
	 * not to allow).
	 * @param on {bool}
	 * @retrn {xml&}
	 */
	virtual xml& xml_multi_root(bool on) = 0;

	/**
	 * Streaming format: call this function in a loop to parse XML data. You can
	 * also parse
	 * complete XML data at once. When reusing this XML object, you should call
	 * reset() function before
	 * parsing next XML data to clear last parse result.
	 * @param data {const char*} xml data.
	 * @return {const char*} When parsing is not complete, returns remaining data.
	 * You can use return value to
	 * get remaining data. When data is '\0', it means parsing is completely
	 * finished.
	 */
	virtual const char* update(const char* data) = 0;

	/**
	 * Determine whether XML parsing is complete.
	 * @param root_tag {const char*} Root node tag name. When NULL string is
	 * passed, this tag name
	 * is used to compare with root node tag name in xml tree to see if they are
	 * the same.
	 * @return {bool}
	 */
	virtual bool complete(const char* root_tag) = 0;

	/**
	 * Reset XML object state. XML object internally automatically manages XML tree
	 * memory. Before reusing this XML object, you need to call this function to
	 * reset internal XML object state and clear last parse result.
	 */
	virtual void reset() = 0;

	/**
	 * Get text content parsed from original XML data.
	 * @return {const string&} Returns reference to result. This reference is
	 * valid, users
	 *  do not need to release it.
	 */
	virtual const string& getText();

	/**
	 * Get node collection of all nodes with a certain tag name from XML tree.
	 * @param tag {const char*} Tag name (case-insensitive).
	 * @return {const std::vector<xml_node*>&} Returns reference to result object.
	 *  When query result is empty, collection is empty, i.e., empty() == true.
	 * Note: xml_node node data in returned collection can be modified, but do not
	 * delete this node,
	 *  because this object internally automatically manages deletion.
	 */
	virtual const std::vector<xml_node*>&
		getElementsByTagName(const char* tag) const = 0;

	/**
	 * Get first xml node object with corresponding tag name from xml tree.
	 * @param tag {const char*} Tag name (case-insensitive).
	 * @return {xml_node*} Returns empty to indicate xml node with this tag name
	 * does not exist.
	 */
	virtual xml_node* getFirstElementByTag(const char* tag) const = 0;

	/**
	 * Get collection of all xml nodes with same hierarchical tag name from xml
	 * tree.
	 * @param tags {const char*} Hierarchical tag name, separated by '/'. For
	 * example, for xml data:
	 * <root> <first> <second> <third name="test1"> text1 </third> </second>
	 * </first> ...
	 * <root> <first> <second> <third name="test2"> text2 </third> </second>
	 * </first> ...
	 * <root> <first> <second> <third name="test3"> text3 </third> </second>
	 * </first> ...
	 * You can use hierarchical tag name root/first/second/third to query all
	 * matching nodes at once.
	 * @return {const std::vector<xml_node*>&} Returns xml node collection, 
	 *  When query result is empty, collection is empty, i.e., empty() == true.
	 * Note: xml_node node data in returned collection can be modified, but do not
	 * delete this node,
	 *  because this object internally automatically manages deletion.
	 */
	virtual const std::vector<xml_node*>&
		getElementsByTags(const char* tags) const = 0;

	/**
	 * Get first xml node with specified hierarchical tag name from xml tree.
	 * @param tags {const char*} Hierarchical tag name, separated by '/'. For
	 * example, for xml data:
	 * <root> <first> <second> <third name="test1"> text1 </third> </second>
	 * </first> ...
	 * <root> <first> <second> <third name="test2"> text2 </third> </second>
	 * </first> ...
	 * <root> <first> <second> <third name="test3"> text3 </third> </second>
	 * </first> ...
	 * You can use hierarchical tag name root/first/second/third to query all
	 * matching nodes at once.
	 * @return {xml_node*} Returns empty to indicate not found.
	 */
	virtual xml_node* getFirstElementByTags(const char* tags) const = 0;

	/**
	 * Get xml node collection with attribute name name and same attribute value
	 * from xml tree.
	 * @param value {const char*} Attribute value with attribute name name.
	 * @return {const std::vector<xml_node*>&} Returns reference to result object.
	 *  When query result is empty, collection is empty, i.e., empty() == true.
	 * Note: xml_node node data in returned collection can be modified, but do not
	 * delete this node,
	 *  because this object internally automatically manages deletion.
	 */
	virtual const std::vector<xml_node*>&
		getElementsByName(const char* value) const = 0;

	/**
	 * Get xml node element collection with specified attribute name and attribute
	 * value from xml tree.
	 * @param name {const char*} Attribute name.
	 * @param value {const char*} Attribute value.
	 * @return {const std::vector<xml_node*>&} Returns reference to result object.
	 *  When query result is empty, collection is empty, i.e., empty() == true.
	 */
	virtual const std::vector<xml_node*>& getElementsByAttr(
		const char* name, const char* value) const = 0;

	/**
	 * Get xml node element with specified id value from xml tree.
	 * @param id {const char*} id value.
	 * @return {const xml_node*} xml node element, returns NULL to indicate not
	 * found.
	 *  Returned xml node does not need to be released.
	 */
	virtual xml_node* getElementById(const char* id) const = 0;

	/**
	 * Create an xml_node node object.
	 * @param tag {const char*} Tag name.
	 * @param txt {const char*} Text string.
	 * @return {xml_node*} Newly created xml_node object. Users do not need to
	 * manually release it, because
	 * when xml object is destroyed, these nodes will be automatically released.
	 * However, users can also
	 *  call reset to release these xml_node node objects when not needed.
	 */
	virtual xml_node& create_node(const char* tag,
		const char* txt = NULL) = 0;

	/**
	 * Create an xml_node node object and simultaneously specify data in input
	 * stream as node's text content.
	 * @param tag {const char*} Tag name.
	 * @param in {istream&} Input stream object.
	 * @param off {size_t} For file stream, specifies start position of data to be
	 * read.
	 * @param len {size_t} Number of bytes to be read. When this value is 0, reads
	 * until stream ends.
	 * @return {xml_node*} Newly created xml_node object. Users do not need to
	 * manually release it, because
	 * when xml object is destroyed, these nodes will be automatically released.
	 * However, users can also
	 *  call reset to release these xml_node node objects when not needed.
	 */
	virtual xml_node& create_node(const char* tag, istream& in,
		size_t off = 0, size_t len = 0) = 0;

	/**
	 * Create an xml_node node object.
	 * @param tag {const char*} Tag name.
	 * @param number {long long int} 64-bit integer.
	 * @return {xml_node*} Newly created xml_node object. Users do not need to
	 * manually release it, because
	 * when xml object is destroyed, these nodes will be automatically released.
	 * However, users can also
	 * call reset to release these xml_node node objects when not needed.
	 */
#if defined(_WIN32) || defined(_WIN64)
	xml_node& create_node(const char* tag, __int64 number);
#else
	xml_node& create_node(const char* tag, long long int number);
#endif

	/**
	 * Get root node object. Note that this node is a virtual node and does not
	 * contain any data.
	 * It is the top-level parent of all xml nodes.
	 * @return {xml_node&}
	 */
	virtual xml_node& get_root() = 0;

	/**
	 * Start traversing xml object and get first node.
	 * @return {xml_node*} Returns empty to indicate xml object is empty node.
	 * Note: Returned node object does not need to be manually released, because
	 * this object
	 *  internally automatically releases it.
	 */
	virtual xml_node* first_node() = 0;

	/**
	 * Get next xml node in xml tree.
	 * @return {xml_node*} Returns empty to indicate traversal finished.
	 * Note: Returned node object does not need to be manually released, because
	 * this object
	 *  internally automatically releases it.
	 */
	virtual xml_node* next_node() = 0;

	/**
	 * Convert xml tree to string.
	 * @param out {string&} Buffer to store converted result.
	 */
	virtual void build_xml(string& out) const { (void) out; };

	/**
	 * Convert xml object to string.
	 * @param len {size_t*} When not NULL, stores data length.
	 * @return {const char*} xml string.
	 */
	virtual const char* to_string(size_t* len = NULL) const = 0;

	/**
	 * Get total memory size currently allocated by xml object.
	 * @return {size_t}
	 */
	virtual size_t space() const = 0;

	/**
	 * Clear memory size counter allocated by xml. Reset to 0.
	 */
	virtual void space_clear() = 0;

	/**
	 * Get number of xml nodes in current xml object.
	 * @return {size_t}
	 */
	virtual size_t node_count() const = 0;

	/**
	 * Get total number of attributes of all xml nodes in current xml object.
	 * @return {size_t}
	 */
	virtual size_t attr_count() const = 0;

public:
	// pipe_stream virtual function implementation.

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear();

protected:
	dbuf_guard dbuf_;
	std::vector<xml_node*> elements_;
	string* buf_;
	//bool dummyRootAdded_;

	ACL_TOKEN* m_pTokenTree;
	//std::list<xml_node*> nodes_tmp_;
};

} // namespace acl

