#ifndef ACL_JSON_INCLUDE_H
#define ACL_JSON_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_dbuf_pool.h"
#include "../stdlib/acl_iterator.h"
#include "../stdlib/acl_vstring.h"
#include "../stdlib/acl_ring.h"
#include "../stdlib/acl_array.h"

typedef struct ACL_JSON ACL_JSON;
typedef struct ACL_JSON_NODE ACL_JSON_NODE;

struct ACL_JSON_NODE {
	ACL_VSTRING *ltag;          /**< tag name */
	ACL_VSTRING *text;          /**< When the node is a leaf
				     *   node, the text content is
				     *   non-empty */
	ACL_JSON_NODE *tag_node;    /**< When the tag value is a
				     *   json node, it is non-empty */
	ACL_JSON_NODE *parent;      /**< parent node */
	ACL_RING children;          /**< child node collection */
	unsigned short type;        /**< node type */
#define	ACL_JSON_T_A_STRING      (1 << 0)
#define	ACL_JSON_T_A_NUMBER      (1 << 1)
#define	ACL_JSON_T_A_BOOL        (1 << 2)
#define	ACL_JSON_T_A_NULL        (1 << 3)
#define	ACL_JSON_T_A_DOUBLE      (1 << 4)

#define	ACL_JSON_T_A_TYPES       (ACL_JSON_T_A_NULL | ACL_JSON_T_A_BOOL | \
	ACL_JSON_T_A_NUMBER | ACL_JSON_T_A_DOUBLE | ACL_JSON_T_A_STRING)

#define	ACL_JSON_T_STRING        (1 << 5)
#define	ACL_JSON_T_NUMBER        (1 << 6)
#define	ACL_JSON_T_BOOL          (1 << 7)
#define	ACL_JSON_T_NULL          (1 << 8)
#define ACL_JSON_T_DOUBLE        (1 << 9)

#define ACL_JSON_T_ARRAY         (1 << 10)
#define ACL_JSON_T_OBJ           (1 << 11)
#define ACL_JSON_T_LEAF          (1 << 12)
#define ACL_JSON_T_MEMBER        (1 << 13)
#define ACL_JSON_T_PAIR          (1 << 14)
#define	ACL_JSON_T_ELEMENT       (1 << 15)

	unsigned short depth;       /**< current node depth */

	/* private */
	unsigned char quote;        /**< If 0 indicates ' or " */
	unsigned char left_ch;      /**< First character of the node: { or [ */
	unsigned char right_ch;     /**< Last character of the node: } or ] */
	unsigned backslash:1;       /**< escape character \ */
	unsigned part_word:1;       /**< Whether the tag value contains a word flag */
	unsigned disabled:1;        /**< Whether this node is disabled */
	ACL_JSON *json;             /**< json object */
	ACL_RING  node;             /**< current node */

	/* public: for acl_iterator, through acl_foreach to
	 * iterate through all child nodes of this node */

	/* Get the head element of the container */
	ACL_JSON_NODE *(*iter_head)(ACL_ITER*, ACL_JSON_NODE*);
	/* Get the next element of the container */
	ACL_JSON_NODE *(*iter_next)(ACL_ITER*, ACL_JSON_NODE*);
	/* Get the tail element of the container */
	ACL_JSON_NODE *(*iter_tail)(ACL_ITER*, ACL_JSON_NODE*);
	/* Get the previous element of the container */
	ACL_JSON_NODE *(*iter_prev)(ACL_ITER*, ACL_JSON_NODE*);
};

enum {
	ACL_JSON_S_ROOT,	/**< root node */
	ACL_JSON_S_OBJ,		/**< tag name and value */
	ACL_JSON_S_MEMBER,
	ACL_JSON_S_ARRAY,	/**< json node array */
	ACL_JSON_S_ELEMENT,
	ACL_JSON_S_PAIR,	/**< name:value pair */
	ACL_JSON_S_NEXT,	/**< next node */
	ACL_JSON_S_TAG,		/**< tag name */
	ACL_JSON_S_VALUE,	/**< node value or content */
	ACL_JSON_S_COLON,	/**< colon : */
	ACL_JSON_S_STRING,
	ACL_JSON_S_STREND
};

struct ACL_JSON {
	int   depth;                /**< maximum depth */
	int   node_cnt;             /**< node count, excluding root node */
	ACL_JSON_NODE *root;        /**< json root node */
	int   finish;               /**< whether parsing is complete */
	unsigned flag;              /**< flag bit */
#define	ACL_JSON_FLAG_PART_WORD	(1 << 0)  /**< whether data contains words */
#define ACL_JSON_FLAG_ADD_SPACE	(1 << 1)  /**< whether to add
					 *   spaces when outputting
					 *   json */

	/* public: for acl_iterator, through acl_foreach to
	 * iterate through all child nodes */

	/* Get the head element of the container */
	ACL_JSON_NODE *(*iter_head)(ACL_ITER*, ACL_JSON*);
	/* Get the next element of the container */
	ACL_JSON_NODE *(*iter_next)(ACL_ITER*, ACL_JSON*);
	/* Get the tail element of the container */
	ACL_JSON_NODE *(*iter_tail)(ACL_ITER*, ACL_JSON*);
	/* Get the previous element of the container */
	ACL_JSON_NODE *(*iter_prev)(ACL_ITER*, ACL_JSON*);

	/* private */

	int   status;               /**< state machine current state */

	ACL_JSON_NODE *curr_node;   /**< current json node being processed */
	ACL_DBUF_POOL *dbuf;        /**< session memory pool object */
	ACL_DBUF_POOL *dbuf_inner;  /**< session memory pool object */
	size_t dbuf_keep;
};

/*----------------------------- in acl_json.c -----------------------------*/

/**
 * Allocate a json node.
 * @param json {ACL_JSON*} json object
 * @return {ACL_JSON_NODE*} json node pointer
 */
ACL_API ACL_JSON_NODE *acl_json_node_alloc(ACL_JSON *json);

/**
 * Delete a json node and its child nodes from the json object, and
 * free the node and child nodes memory space. The memory pool
 * function will free the memory space of the json node.
 * @param node {ACL_JSON_NODE*} json node
 * @return {int} Number of deleted nodes
 */
ACL_API int acl_json_node_delete(ACL_JSON_NODE *node);

/**
 * Delete the next node from the current json node, and return
 * the next json node.
 * @param node {ACL_JSON_NODE*} json node
* @return {ACL_JSON_NODE*} Returns the next json node
 */
ACL_API ACL_JSON_NODE *acl_json_node_erase(ACL_JSON_NODE *node, ACL_ITER *it);

/**
 * Delete the previous node from the current json node, and
 * return the previous json node.
 * @param node {ACL_JSON_NODE*} json node
* @return {ACL_JSON_NODE*} Returns the previous json node
 */
ACL_API ACL_JSON_NODE *acl_json_node_rerase(ACL_JSON_NODE *node, ACL_ITER *it);

/**
 * Disable/enable a json node. Disabled nodes will not be added
 * when building json strings, but when parsing json they can still
 * be used, to allow re-parsing
 * @param node {ACL_JSON_NODE*} json node
 * @param yes {int} Whether to disable the json node
 */
ACL_API void acl_json_node_disable(ACL_JSON_NODE *node, int yes);

/**
 * Check if the specified json node has been disabled.
 * @param node {ACL_JSON_NODE*} json node
 * @return {int} If 0 indicates not disabled, otherwise indicates
 *  disabled state, non-zero indicates disabled
 */
ACL_API int acl_json_node_disabled(ACL_JSON_NODE *node);

/**
 * Append a sibling node to a json node (the sibling node must be
 * a detached json node)
 * @param node1 {ACL_JSON_NODE*} The node to append to
 * @param node2 {ACL_JSON_NODE*} The sibling json node to append
 */
ACL_API void acl_json_node_append(ACL_JSON_NODE *node1, ACL_JSON_NODE *node2);

/**
 * Add a json node as a child node to a json node.
 * @param parent {ACL_JSON_NODE*} Parent node
 * @param child {ACL_JSON_NODE*} Child node
 */
ACL_API void acl_json_node_add_child(ACL_JSON_NODE *parent, ACL_JSON_NODE *child);

/**
 * Copy a JSON node from a JSON object to a JSON node in the JSON
 * object, and create a new JSON node at the target location
 * @param json {ACL_JSON*} Target JSON object
 * @param from {ACL_JSON_NODE*} A JSON node from the source JSON object
 * @return {ACL_JSON_NODE*} Returns a non-empty pointer
 */
ACL_API ACL_JSON_NODE *acl_json_node_duplicate(ACL_JSON *json, ACL_JSON_NODE *from);

/**
 * Get the parent node of a json node.
 * @param node {ACL_JSON_NODE*} json node
 * @return {ACL_JSON_NODE*} Parent node, if NULL indicates its
 *  parent node does not exist
 */
ACL_API ACL_JSON_NODE *acl_json_node_parent(ACL_JSON_NODE *node);

/**
 * Get the next sibling node of a json node.
 * @param node {ACL_JSON_NODE*} json node
 * @return {ACL_JSON_NODE*} The next sibling node of the json node,
 *  if NULL indicates it does not exist
 */
ACL_API ACL_JSON_NODE *acl_json_node_next(ACL_JSON_NODE *node);

/**
 * Get the previous sibling node of a json node.
 * @param node {ACL_JSON_NODE*} json node
 * @return {ACL_JSON_NODE*} The previous sibling node of the json
 *  node, if NULL indicates it does not exist
 */
ACL_API ACL_JSON_NODE *acl_json_node_prev(ACL_JSON_NODE *node);

/**
 * In the process of iterating through Json objects, delete the
 * json node pointed to by the current iterator and return the next
 * json node
 * @param json {ACL_JSON*} json object
 * @param it {ACL_ITER*} Iterator for json objects
 * @return {ACL_JSON_NODE*} Returns the next json node
 */
ACL_API ACL_JSON_NODE *acl_json_erase(ACL_JSON *json, ACL_ITER *it);

/**
 * Create a json object.
 * @return {ACL_JSON*} Newly created json object
 */
ACL_API ACL_JSON *acl_json_alloc(void);

/**
 * Create a json object.
 * @param dbuf {ACL_DBUF_POOL*} Memory pool object, if NULL, the json object
 *  will allocate node memory on the heap, otherwise it will
 *  automatically use the json object's memory pool
 * @return {ACL_JSON*} Newly created json object
 */
ACL_API ACL_JSON *acl_json_dbuf_alloc(ACL_DBUF_POOL *dbuf);

/**
 * Create a new JSON object from a JSON node in a JSON object.
 * @param node {ACL_JSON_NODE*} A JSON node from the source JSON object
 * @return {ACL_JSON*} Newly created JSON object
 */
ACL_API ACL_JSON *acl_json_create(ACL_JSON_NODE *node);

/**
 * Create a new JSON object from a JSON node in a JSON object.
 * @param dbuf {ACL_DBUF_POOL*} Memory pool object, if NULL, the json object
 *  will allocate node memory on the heap, otherwise it will
 *  automatically use the json object's memory pool
 * @param node {ACL_JSON_NODE*} A JSON node from the source JSON object
 * @return {ACL_JSON*} Newly created JSON object
 */
ACL_API ACL_JSON *acl_json_dbuf_create(ACL_DBUF_POOL *dbuf, ACL_JSON_NODE *node);

/**
 * Set a certain ACL_JSON_NODE node as the root node of a json object,
 * so that you can easily iterate through all child nodes of
 * this node (in the iteration process, the root node itself is
 * not included in the iteration). This iteration method lists
 * nodes when iterating through a certain ACL_JSON_NODE node,
 * it can iterate through all child nodes in order
 * @param json {ACL_JSON*} json object
 * @param node {ACL_JSON_NODE*} ACL_JSON_NODE node
 */
ACL_API void acl_json_foreach_init(ACL_JSON *json, ACL_JSON_NODE *node);

/**
 * Free a json object, and also free all json nodes created by the object.
 * @param json {ACL_JSON*} json object
 */
ACL_API void acl_json_free(ACL_JSON *json);

/**
 * Reset json object state.
 * @param json {ACL_JSON*} json object
 */
ACL_API void acl_json_reset(ACL_JSON *json);

/*------------------------- in acl_json_parse.c ---------------------------*/

/**
 * Parse json data, and automatically create json nodes.
 * @param json {ACL_JSON*} json object
 * @param data {const char*} A string ending with '\0', which
 *  can be a complete json string; it can also be an
 *  incomplete json string, and you can call this function in
 *  a loop, and when the data is complete, it will be parsed;
 *  this parameter cannot be NULL, otherwise it will directly
 *  return an empty string. This parameter is forbidden to be
 *  NULL
 * @return {const char*} After parsing is complete, this return
 *  value indicates the pointer to the remaining data
 */
ACL_API const char* acl_json_update(ACL_JSON *json, const char *data);

/**
 * Check if JSON parsing is complete.
 * @param json {ACL_JSON*} json object
 * @return {int} Returns a non-zero value to indicate parsing
 *  is complete, otherwise indicates not complete
 */
ACL_API int acl_json_finish(ACL_JSON *json);

/*------------------------- in acl_json_util.c ----------------------------*/

/**
 * Get the first json node with the same tag name from the json object.
 * @param json {ACL_JSON*} json object
 * @param tag {const char*} Tag name
 * @return {ACL_JSON_NODE*} Returns the found json node, if NULL
 *  indicates no matching json node was found
 */
ACL_API ACL_JSON_NODE *acl_json_getFirstElementByTagName(
	ACL_JSON *json, const char *tag);

/**
 * Free the dynamic array returned by functions such as
 * acl_json_getElementsByTagName, acl_json_getElementsByName,
 * etc. Because the elements in this dynamic array are all
 * pointers to elements in the ACL_JSON object, when freeing
 * this dynamic array, as long as ACL_JSON is not freed, the
 * elements originally in the array can still be used.
 * This function only frees the xml node elements
 * @param a {ACL_ARRAY*} Dynamic array pointer
 */
ACL_API void acl_json_free_array(ACL_ARRAY *a);

/**
 * Get all json nodes with the same tag name from the json object collection.
 * @param json {ACL_JSON*} json object
 * @param tag {const char*} Tag name
 * @return {ACL_ARRAY*} Returns the found json node
 *  collection, which is a dynamic array, if NULL indicates no
 *  matching json node was found, non-empty values need to be
 *  freed with acl_json_free_array
 */
ACL_API ACL_ARRAY *acl_json_getElementsByTagName(
	ACL_JSON *json, const char *tag);

/**
 * Get all json nodes with the same multi-level tag name from
 * the json object collection.
 * @param json {ACL_JSON*} json object
 * @param tags {const char*} Multi-level tag name, separated
 *  by '/' to form a tag path, for example json data:
 *  { 'root': [
 *      'first': { 'second': { 'third': 'test1' } },
 *      'first': { 'second': { 'third': 'test2' } },
 *      'first': { 'second': { 'third': 'test3' } }
 *    ]
 *  }
 *  You can use the multi-level tag path
 *  root/first/second/third to find all matching nodes at once
 * @return {ACL_ARRAY*} Returns the found json node
 *  collection, which is a dynamic array, if NULL indicates no
 *  matching json node was found, non-empty values need to be
 *  freed with acl_json_free_array
 */
ACL_API ACL_ARRAY *acl_json_getElementsByTags(
	ACL_JSON *json, const char *tags);

/**
 * When creating json objects, create json leaf nodes.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param name {const char*} Tag name, non-empty
 * @param value {const char*} Tag value, non-empty
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_text(ACL_JSON *json,
	const char *name, const char *value);
#define acl_json_create_leaf acl_json_create_text

/**
 * When creating json objects, create json boolean type leaf nodes.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param name {const char*} Tag name, non-empty
 * @param value {int} Boolean value
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_bool(ACL_JSON *json,
	const char *name, int value);

/**
 * When creating json objects, create json null type leaf nodes.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param name {const char*} Tag name, non-empty
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_null(ACL_JSON *json, const char *name);

/**
 * When creating json objects, create json int type leaf nodes.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param name {const char*} Tag name, non-empty
 * @param value {acl_int64} Integer value
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_int64(ACL_JSON *json,
	const char *name, acl_int64 value);

/**
 * When creating json objects, create json double type leaf nodes.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param name {const char*} Tag name, non-empty
 * @param value {double} Floating point value
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_double(ACL_JSON *json,
	const char *name, double value);

/**
 * When creating json objects, create json double type leaf nodes.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param name {const char*} Tag name, non-empty
 * @param value {double} Floating point value
 * @param precision {int} Decimal point precision, when > 0,
 *  it takes effect, default value is 4
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_double2(ACL_JSON *json,
	const char *name, double value, int precision);

/**
 * Create json array string nodes, according to json
 * specification, this node can only add string type elements.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param text {const char*}
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_text(ACL_JSON *json,
	const char *text);

/**
 * Create json array value nodes, according to json
 * specification, this node can only add numeric type
 * elements.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param value {acl_int64}
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_int64(ACL_JSON *json,
	acl_int64 value);
/**
 * Create json array value nodes, according to json
 * specification, this node can only add numeric type
 * elements.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param value {double}
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_double(ACL_JSON *json,
	double value);

/**
 * Create json array boolean nodes, according to json
 * specification, this node can only add boolean type
 * elements.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param value {int} If 0 indicates true, otherwise indicates false
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_bool(ACL_JSON *json, int value);

/**
 * Create json array null nodes, according to json
 * specification, this node can only add null type elements.
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_null(ACL_JSON *json);

/**
 * When creating json objects, create json objects (objects wrapped in {}).
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_obj(ACL_JSON *json);

/**
 * When creating json objects, create json arrays (objects wrapped in []).
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_array(ACL_JSON *json);

/**
 * When creating json objects, create json node pairs (e.g.
 * tagname: ACL_JSON_NODE)
 * @param json {ACL_JSON*} Created by acl_json_alloc / acl_json_alloc1
 * @param name {const char*} json node's tag name
 * @param value {ACL_JSON_NODE*} json node's tag value
 * @return {ACL_JSON_NODE*} The newly created node will be
 *  freed when the ACL_JSON object is freed, so there is no
 *  need to manually free it
 */
ACL_API ACL_JSON_NODE *acl_json_create_node(ACL_JSON *json,
	const char *name, ACL_JSON_NODE *value);

/**
 * When creating json objects, add child nodes to json nodes
 * created by acl_json_create_obj or acl_json_create_array
 * child nodes can be created by the following interfaces:
 * acl_json_create_leaf, acl_json_create_obj, acl_json_create_array
 */
ACL_API void acl_json_node_append_child(ACL_JSON_NODE *parent,
	ACL_JSON_NODE *child);

/**
 * Convert a JSON node in json object to string format.
 * @param node {ACL_JSON_NODE*} json node pointer
 * @param buf {ACL_VSTRING*} Buffer to store the result. If
 *  this parameter is NULL, the internal will automatically
 *  allocate a buffer. If the user needs to free it, it is a
 *  non-empty buffer allocated internally, otherwise it will
 *  directly use the storage space provided by the user
 * @return {ACL_VSTRING*} Buffer to store the converted string
 *  format of the json node. This return value is always
 *  non-empty. Users can determine whether the content is empty
 *  through ACL_VSTRING_LEN(x). If the returned ACL_VSTRING
 *  pointer is allocated internally by this function, the user
 *  needs to
 *  call acl_vstring_free to free it
 */
ACL_API ACL_VSTRING *acl_json_node_build(ACL_JSON_NODE *node, ACL_VSTRING *buf);

/**
 * Convert json object to string format.
 * @param json {ACL_JSON*} json object
 * @param buf {ACL_VSTRING*} Buffer to store the result. If
 *  this parameter is NULL, the internal will automatically
 *  allocate a buffer. If the user needs to free it, it is a
 *  non-empty buffer allocated internally, otherwise it will
 *  directly use the storage space provided by the user
 * @return {ACL_VSTRING*} Buffer to store the converted string
 *  format of the json object. This return value is always
 *  non-empty. Users can determine whether the content is empty
 *  through ACL_VSTRING_LEN(x). The returned ACL_VSTRING
 *  pointer is allocated internally by this function. If the
 *  user needs to free it, call acl_vstring_free to free it
 */
ACL_API ACL_VSTRING *acl_json_build(ACL_JSON *json, ACL_VSTRING *buf);

/**
 * Stream JSON object to string conversion process. This
 * function converts JSON object to string in the process,
 * converts while calling the callback function to notify the
 * caller, and the caller can limit the length and limit the
 * user memory usage. This function is suitable when the JSON
 * object converted to string is very long (e.g. exceeding 100
 * MB), because it uses streaming conversion format, so it
 * does not need to allocate a large amount of memory at once
 * @param json {ACL_JSON*} json object
 * @param length {size_t} During the process of converting to
 *  string, when the length exceeds the length set by the user,
 *  the callback
 *  function will be called
 * @param callback {int (*)(ACL_JSON*, ACL_VSTRING*, void*)}
 *  User-defined callback function. When the second parameter
 *  of the callback function is NULL, it indicates the
 *  conversion is complete. If the user returns
 *  a value < 0 in a certain callback, the conversion will stop
 * @param ctx {void*} One of the parameters for the callback function
 */
ACL_API void acl_json_building(ACL_JSON *json, size_t length,
	int (*callback)(ACL_JSON *, ACL_VSTRING *, void *), void *ctx);

#ifdef __cplusplus
}
#endif

#endif
