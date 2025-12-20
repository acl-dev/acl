#ifndef ACL_XML3_INCLUDE_H
#define ACL_XML3_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_array.h"
#include "../stdlib/acl_ring.h"
#include "../stdlib/acl_vstream.h"
#include "../stdlib/acl_htable.h"
#include "../stdlib/acl_vstring.h"
#include "../stdlib/acl_iterator.h"
#include "../stdlib/acl_dbuf_pool.h"

typedef struct ACL_XML3	ACL_XML3;
typedef struct ACL_XML3_NODE	ACL_XML3_NODE;
typedef struct ACL_XML3_ATTR	ACL_XML3_ATTR;

struct ACL_XML3_ATTR {
	ACL_XML3_NODE *node;            /**< parent node */
	char *name;                     /**< attribute name */
	char *value;                    /**< attribute value */

	size_t name_size;               /**< attribute name length */
	size_t value_size;              /**< attribute value length */

	/* private */
	int   quote;                    /**< If 0 indicates ' or " */
	int   backslash;                /**< escape character \ */
	int   slash;                    /**< whether '/' flag is set */
};

struct ACL_XML3_NODE {
	char *ltag;                     /**< left tag name */
	char *rtag;                     /**< right tag name */
	size_t ltag_size;               /**< left tag name length */
	size_t rtag_size;               /**< right tag name length */
	const char *id;                 /**< ID identifier, only when xml->id_table
					  contains the node's id is non-empty */
	char *text;                     /**< text content representation */
	size_t text_size;               /**< file data length */

	ACL_ARRAY *attr_list;           /**< attribute (ACL_XML3_ATTR) list */
	ACL_XML3_NODE *parent;          /**< parent node */
	ACL_RING children;              /**< child node collection */
	int  depth;                     /**< current node depth */

	/* private */
	ACL_XML3 *xml;                  /**< xml object */
	ACL_RING node;                  /**< current node */
	ACL_XML3_ATTR *curr_attr;       /**< currently being processed attribute */
	int   quote;                    /**< If 0 indicates ' or " */
	int   last_ch;                  /**< last character value
					 *   recorded for the node */
	int   nlt;                      /**< '<' count */
	char  meta[3];                  /**< meta data temporary storage */

	unsigned int flag;
#define	ACL_XML3_F_META_QM	(1 << 0)    /**< '?' flag */
#define	ACL_XML3_F_META_CM	(1 << 1)    /**< '!--' flag */
#define	ACL_XML3_F_META_EM	(1 << 2)    /**< only '!' flag */
#define ACL_XML3_F_SELF_CL	(1 << 3)    /**< self closed flag */
#define	ACL_XML3_F_LEAF		(1 << 4)    /**< leaf node has no child node */

/**< whether it is meta data */
#define	ACL_XML3_F_META		\
	(ACL_XML3_F_META_QM | ACL_XML3_F_META_CM | ACL_XML3_F_META_EM)

#define	ACL_XML3_IS_COMMENT(x)	(((x)->flag & ACL_XML3_F_META_CM))

	int   status;                   /**< state machine current state */
#define ACL_XML3_S_NXT	0               /**< next node */
#define ACL_XML3_S_LLT	1               /**< left '<' */
#define ACL_XML3_S_LGT	2               /**< left '>' */
#define	ACL_XML3_S_LCH	3               /**< left '<' next character */
#define ACL_XML3_S_LEM	4               /**< left '<' then '!' */
#define ACL_XML3_S_LTAG	5               /**< left tag name */
#define ACL_XML3_S_RLT	6               /**< right '<' */
#define ACL_XML3_S_RGT	7               /**< right '>' */
#define ACL_XML3_S_RTAG	8               /**< right tag name */
#define ACL_XML3_S_ATTR	9               /**< tag attribute name */
#define ACL_XML3_S_AVAL	10              /**< tag attribute value */
#define ACL_XML3_S_TXT	11              /**< node text */
#define ACL_XML3_S_MTAG	12              /**< meta data tag */
#define ACL_XML3_S_MTXT	13              /**< meta data text */
#define ACL_XML3_S_MCMT	14              /**< meta data comment */
#define ACL_XML3_S_MEND	15              /**< meta data end */

	/* for acl_iterator, through acl_foreach to iterate through all child nodes */

	/* Get the head element of the container */
	ACL_XML3_NODE *(*iter_head)(ACL_ITER*, ACL_XML3_NODE*);
	/* Get the next element of the container */
	ACL_XML3_NODE *(*iter_next)(ACL_ITER*, ACL_XML3_NODE*);
	/* Get the tail element of the container */
	ACL_XML3_NODE *(*iter_tail)(ACL_ITER*, ACL_XML3_NODE*);
	/* Get the previous element of the container */
	ACL_XML3_NODE *(*iter_prev)(ACL_ITER*, ACL_XML3_NODE*);
};

struct ACL_XML3 {
	/* public */
					
	int   depth;                    /**< maximum depth */
	int   node_cnt;                 /**< node count, excluding root node */
	int   root_cnt;                 /**< root node count */
	ACL_XML3_NODE *root;            /**< XML root node */

	/* private */
	char  addr[1];
	ACL_HTABLE *id_table;           /**< id identifier hash table */
	ACL_XML3_NODE *curr_node;       /**< current XML node being processed */
	ACL_DBUF_POOL *dbuf;            /**< memory pool object */
	ACL_DBUF_POOL *dbuf_inner;      /**< internal partial memory pool object */
	size_t dbuf_keep;               /**< memory pool reserved length */

	unsigned flag;                  /**< flag bit: ACL_XML3_FLAG_xxx */ 

	/**< whether to allow multiple root nodes in one xml
	 *   document, internal default is allowed */
#define	ACL_XML3_FLAG_MULTI_ROOT	(1 << 0)

	/**< whether to ignore the '/' flag in the node */
#define	ACL_XML3_FLAG_IGNORE_SLASH	(1 << 1)

	/* for acl_iterator, through acl_foreach to iterate through all child nodes */

	/* Get the head element of the container */
	ACL_XML3_NODE *(*iter_head)(ACL_ITER*, ACL_XML3*);
	/* Get the next element of the container */
	ACL_XML3_NODE *(*iter_next)(ACL_ITER*, ACL_XML3*);
	/* Get the tail element of the container */
	ACL_XML3_NODE *(*iter_tail)(ACL_ITER*, ACL_XML3*);
	/* Get the previous element of the container */
	ACL_XML3_NODE *(*iter_prev)(ACL_ITER*, ACL_XML3*);
};

/****************************************************************************/
/*                  High-level interfaces, users can directly use
 *                  these interfaces                  */
/****************************************************************************/

/*----------------------------- in acl_xml3.c ------------------------------*/

/**
 * Determine if an xml object is closed, i.e., whether all
 * tags are matched. If the xml object's xml node elements are
 * empty, it is also considered closed.
 * @param xml {ACL_XML3*} xml object
 * @return {int} 0: No; 1: Yes
 */
ACL_API int acl_xml3_is_closure(ACL_XML3 *xml);

/**
 * Determine if an xml object is complete based on the specified
 * tag name. This tag name is a child node of xml object's root.
 * When the last xml node's tag name matches, it is considered
 * that the xml object is complete. To ensure the correctness of
 * the judgment, the caller should ensure that the root node has
 * only one, and xml->root has only one child node, otherwise
 * the judgment may be incorrect.
 * @param xml {ACL_XML3*} xml object
 * @param tag {const char*} User-specified tag name, internal
 *  matching is case-insensitive
 * @return {int} 0: No; 1: Yes
 */
ACL_API int acl_xml3_is_complete(ACL_XML3 *xml, const char *tag);

/**
 * Create an xml object.
 * @return {ACL_XML3*} The newly created xml object
 */
ACL_API ACL_XML3 *acl_xml3_alloc(void);

/**
 * Create an xml object, all internal memory allocations for this
 * xml object are performed on this memory pool.
 * @param dbuf {ACL_DBUF_POOL*} Memory pool object, when this
 *  parameter is NULL, the xml object will allocate memory for
 *  all nodes on the memory pool, otherwise it will
 *  automatically allocate memory for the xml object.
 * @return {ACL_XML3*} The newly created xml object
 */
ACL_API ACL_XML3 *acl_xml3_dbuf_alloc(ACL_DBUF_POOL *dbuf);

/**
 * Set a certain ACL_XML3_NODE node as the root node of an XML
 * object, so that it can be conveniently traversed through the
 * node's child nodes (in this traversal, all nodes are not the
 * node's descendants). This traversal-style list node when
 * traversing a certain ACL_XML3_NODE node may only traverse one
 * child node.
 * @param xml {ACL_XML3*} xml object
 * @param node {ACL_XML3_NODE*} AXL_XML_NODE node
 */
ACL_API void acl_xml3_foreach_init(ACL_XML3 *xml, ACL_XML3_NODE *node);

/**
 * Set whether an xml document allows multiple root xml nodes,
 * internal default supports multiple root nodes.
 * @param xml {ACL_XML3*} xml object
 * @param on {int} If 0, indicates not allowed; if 0, indicates
 *  allowed. When stopping the judgment of multiple root xml
 *  nodes, when parsing ends, if there is more than one root node,
 *  it will return the remaining data.
 */
ACL_API void acl_xml3_multi_root(ACL_XML3 *xml, int on);

/**
 * Set XML root node parsing, whether to ignore the /, e.g.:
 * <test id=111>, <test id=111 />, both of these without /
 * are written are legal, but only the second writing is legal,
 * this function is for compatibility, if the first writing is
 * not ignored, parsing will fail.
 * @param xml {ACL_XML3*} xml object
 * @param ignore {int} If 0, indicates not to ignore the / in the node
 */
ACL_API void acl_xml3_slash(ACL_XML3 *xml, int ignore);

/**
 * Free an xml object, and free all xml nodes created by this object.
 * @param xml {ACL_XML3*} xml object
 * @return {int} The number of freed xml nodes
 */
ACL_API int acl_xml3_free(ACL_XML3 *xml);

/**
 * Reset XML object state.
 * @param xml {ACL_XML3*} xml object
 */
ACL_API void acl_xml3_reset(ACL_XML3 *xml);

/*------------------------- in acl_xml3_parse.c ----------------------------*/

/**
 * Parse xml data, and automatically create xml nodes.
 * @param xml {ACL_XML3*} xml object
 * @param data {char*} A string ending with '\0', which can be a
 *  complete xml document; it can also be an incomplete xml
 *  document, and this function can be called in a loop, and the
 *  data can be parsed incrementally.
 * @return {char*} If through acl_xml3_multi_root allows multiple
 *  root xml nodes, when multiple xml nodes are found, the address
 *  returned by this function will be set to '\0'; otherwise
 *  return the remaining data address
 *  which is a non-empty string.
 *  Note: You can also use acl_xml3_is_complete to determine
 *  whether it is complete.
 */
ACL_API char *acl_xml3_update(ACL_XML3 *xml, char *data);
#define	acl_xml3_parse	acl_xml3_update

/*------------------------- in acl_xml3_util.c -----------------------------*/

/**
 * Initialize some self-closing tags such as input, br, hr, forming
 * a self-closing tag list, for acl_xml3_tag_selfclosed to query,
 * to determine whether a tag is a built-in self-closing tag,
 * this function can only be initialized once, or not initialized.
 */
ACL_API void acl_xml3_tag_init(void);

/**
 * Add some self-closing tags defined by the user.
 * @param tag {const char*} tag name, note that the tag length
 *  should not exceed 254 bytes
 */
ACL_API void acl_xml3_tag_add(const char *tag);

/**
 * Query the self-closing tag list initialized by
 * acl_xml3_tag_init, you can call this function to determine
 * whether a tag is a self-closing tag, if acl_xml3_tag_init has
 * not been called, this function will automatically return 0.
 * @param tag {const char*} tag name
 * @return {int} 0: indicates no; 1: indicates yes
 */
ACL_API int  acl_xml3_tag_selfclosed(const char *tag);

/**
 * Determine whether a tag name xml node is a leaf node, leaf
 * nodes have no child nodes.
 * @param tag {const char*} tag name
 * @return {int} 0: not a leaf node; 1: is a leaf node
 */
ACL_API int  acl_xml3_tag_leaf(const char *tag);

/**
 * Free the dynamic array returned by
 * acl_xml3_getElementsByTagName, acl_xml3_getElementsByName,
 * acl_xml3_getElementsByAttr and other functions, because the
 * elements in this dynamic array are all references to ACL_XML3
 * object elements, so when freeing this dynamic array, as long as
 * ACL_XML3
 * is not freed, the elements originally in this array can still be used.
 * This function does not free the xml node elements.
 * @param a {ACL_ARRAY*} dynamic array object
 */
ACL_API void acl_xml3_free_array(ACL_ARRAY *a);

/**
 * Get the first xml node with the same tag name from the xml object.
 * @param xml {ACL_XML3*} xml object
 * @param tag {const char*} tag name
 * @return {ACL_XML3_NODE*} The found xml node, returns NULL if
 *  no matching xml node is found.
 */
ACL_API ACL_XML3_NODE *acl_xml3_getFirstElementByTagName(
	ACL_XML3 *xml, const char *tag);

/**
 * Get all xml nodes with the same tag name from the xml object.
 * @param xml {ACL_XML3*} xml object
 * @param tag {const char*} tag name
 * @return {ACL_ARRAY*} The found xml node collection, is a
 *  dynamic array, returns NULL if no matching xml node is found,
 *  non-NULL value needs to be freed with acl_xml3_free_array.
 */
ACL_API ACL_ARRAY *acl_xml3_getElementsByTagName(
	ACL_XML3 *xml, const char *tag);

/**
 * Get all xml nodes with the same multi-level tag name from the xml object.
 * @param xml {ACL_XML3*} xml object
 * @param tags {const char*} Multi-level tag name, separated by
 *  '/' to form a tag path, e.g. xml data:
 *  <root> <first> <second> <third name="test1"> text1 </third>
 *  </second> </first>
 *  <root> <first> <second> <third name="test2"> text2 </third>
 *  </second> </first>
 *  <root> <first> <second> <third name="test3"> text3 </third>
 *  </second> </first>
 *  You can use the multi-level tag name root/first/second/third
 *  to find all matching nodes at once.
 * @return {ACL_ARRAY*} The found xml node collection, is a
 *  dynamic array, returns NULL if no matching xml node is found,
 *  non-NULL value needs to be freed with acl_xml3_free_array.
 */
ACL_API ACL_ARRAY *acl_xml3_getElementsByTags(ACL_XML3 *xml, const char *tags);

/**
 * Get all xml node elements with the same name attribute value
 * from the xml object.
 * @param xml {ACL_XML3*} xml object
 * @param value {const char*} attribute value with name attribute
 * @return {ACL_ARRAY*} The found xml node collection, is a
 *  dynamic array, returns NULL if no matching xml node is found,
 *  non-NULL value needs to be freed with acl_xml3_free_array.
 */
ACL_API ACL_ARRAY *acl_xml3_getElementsByName(ACL_XML3 *xml, const char *value);

/**
 * Get all xml node elements with the same attribute name and value
 * from the xml object.
 * @param xml {ACL_XML3*} xml object
 * @param name {const char*} attribute name
 * @param value {const char*} attribute value
 * @return {ACL_ARRAY*} The found xml node collection, is a
 *  dynamic array, returns NULL if no matching xml node is found,
 *  non-NULL value needs to be freed with acl_xml3_free_array.
 */
ACL_API ACL_ARRAY *acl_xml3_getElementsByAttr(ACL_XML3 *xml,
	const char *name, const char *value);

/**
 * Get a certain attribute object of an xml node element with the
 * specified id value from the xml object.
 * @param xml {ACL_XML3*} xml object
 * @param id {const char*} id value
 * @return {ACL_XML3_ATTR*} A certain attribute object of an
 *  xml node, returns NULL if
 *  no matching attribute is found, the return value does not need to be freed.
 */
ACL_API ACL_XML3_ATTR *acl_xml3_getAttrById(ACL_XML3 *xml, const char *id);

/**
 * Get a certain attribute value of an xml node element with the
 * specified id value from the xml object.
 * @param xml {ACL_XML3*} xml object
 * @param id {const char*} id value
 * @return {const char*} A certain attribute value of an xml
 *  node, returns NULL if not found
 *  matching attribute.
 */
ACL_API const char *acl_xml3_getAttrValueById(ACL_XML3 *xml, const char *id);

/**
 * Get an xml node element with the specified id value from the xml object.
 * @param xml {ACL_XML3*} xml object
 * @param id {const char*} id value
 * @return {ACL_XML3_NODE*} xml node element, returns NULL if not found.
 *  Returns an xml node, the return value does not need to be freed.
 */
ACL_API ACL_XML3_NODE *acl_xml3_getElementById(ACL_XML3 *xml, const char *id);

/**
 * Get a node starting with ? ! etc. from the xml object.
 * @param xml {ACL_XML3*} xml object
 * @param tag {const char*} tag name
 * @return {ACL_XML3_NODE*} xml node element, returns NULL if not found.
 *  Returns an xml node, the return value does not need to be freed.
 */
ACL_API ACL_XML3_NODE *acl_xml3_getElementMeta(ACL_XML3 *xml, const char *tag);

/**
 * Get the encoding format of the xml object string.
 * @param xml {ACL_XML3*} xml object
 * @return {const char*} The encoding format string, returns NULL if not set.
 */
ACL_API const char *acl_xml3_getEncoding(ACL_XML3 *xml);

/**
 * Get the content type of the xml object, e.g., text/xsl.
 * @param xml {ACL_XML3*} xml object
 * @return {const char*} Returns NULL if not set.
 */
ACL_API const char *acl_xml3_getType(ACL_XML3 *xml);

/**
 * Get the attribute object of a specific attribute name from an xml node.
 * @param node {ACL_XML3_NODE*} xml node
 * @param name {const char*} attribute name
 * @return {ACL_XML3_ATTR*} attribute object, returns NULL if
 *  not found, the return value does not need to be freed.
 */
ACL_API ACL_XML3_ATTR *acl_xml3_getElementAttr(ACL_XML3_NODE *node,
		const char *name);

/**
 * Get the attribute value of a specific attribute name from an xml node.
 * @param node {ACL_XML3_NODE*} xml node
 * @param name {const char*} attribute name
 * @return {const char*} attribute value, returns NULL if not found.
 */
ACL_API const char *acl_xml3_getElementAttrVal(
			ACL_XML3_NODE *node, const char *name);

/**
 * Delete a specific attribute from an xml node. If the attribute
 * is an id attribute, it will also be deleted from xml->id_table.
 * @param node {ACL_XML3_NODE*} xml node
 * @param name {const char*} attribute name
 * @return {int} 0 indicates successful deletion, -1 indicates
 *  deletion failed (possibly due to the attribute not existing).
 */
ACL_API int acl_xml3_removeElementAttr(ACL_XML3_NODE *node, const char *name);

#if 0
/**
 * Add an attribute to an xml node. If the attribute already
 * exists, the new attribute value will replace the old one,
 * and a new attribute object will be returned.
 * @param node {ACL_XML3_NODE*} xml node
 * @param name {const char*} attribute name
 * @param value {const char*} attribute value
 * @return {ACL_XML3_ATTR*} The modified attribute object
 *  (possibly the original one, or a new one), 
 *  the return value does not need to be freed.
 */
ACL_API ACL_XML3_ATTR *acl_xml3_addElementAttr(ACL_XML3_NODE *node,
		const char *name, const char *value);

/**
 * Create a node with the tag name and text content as an xml
 * node. This function is mainly used when building xml objects.
 * @param xml {ACL_XML3*} xml object, this object should be
 *  created by acl_xml3_alloc function.
 * @param tagname {const char*} tag name, must be a non-empty
 *  string with length > 0
 * @param text {const char*} node text content, can be empty
 * @return {ACL_XML3_NODE*} The newly created xml node, this
 *  function will never return NULL, and whether the return value
 *  is NULL will cause internal automatic memory allocation.
 */
ACL_API ACL_XML3_NODE *acl_xml3_create_node(ACL_XML3 *xml,
	const char *tagname, const char *text);

/**
 * Add an attribute to an xml node. This function is mainly used
 * when building xml objects.
 * @param node {ACL_XML3_NODE*} Node created by acl_xml3_create_node
 * @param name {const char*} attribute name, must be a
 *  non-empty string with length > 0
 * @param value {const char*} attribute value, can be empty
 * @return {ACL_XML3_ATTR*} xml node attribute object, when the
 *  return value is not NULL, this function
 *  will automatically allocate memory internally.
 */
ACL_API ACL_XML3_ATTR *acl_xml3_node_add_attr(ACL_XML3_NODE *node,
	const char *name, const char *value);

/**
 * Add multiple attributes to an xml node. This function is
 * mainly used when building xml objects.
 * @param node {ACL_XML3_NODE*} Node created by acl_xml3_create_node
 * @param ... Multiple attributes, ending with NULL, e.g.:
 *  {name1}, {value1}, {name2}, {value2}, ... NULL
 */
ACL_API void acl_xml3_node_add_attrs(ACL_XML3_NODE *node, ...);

/**
 * Set the text content of an xml node. This function is mainly
 * used when building xml objects.
 * @param node {ACL_XML3_NODE*} Node created by acl_xml3_create_node
 * @param text {const char*} text content
 */
ACL_API void acl_xml3_node_set_text(ACL_XML3_NODE *node, const char *text);

/**
 * Convert an xml object to a string.
 * @param xml {ACL_XML3*} xml object
 * @param buf {ACL_VSTRING*} Buffer to store the converted
 *  result. If this parameter is NULL, an internal buffer will be
 *  automatically created, and the caller needs to free it.
 *  Otherwise, the internal buffer
 *  will be used directly to store the result.
 * @return {ACL_VSTRING*} The buffer storing the string
 *  converted from the xml object. The return value is always
 *  non-NULL. The caller can determine if the content is empty
 *  by ACL_VSTRING_LEN(x). If the returned ACL_VSTRING pointer
 *  is an internally created one, the user needs to free it
 *  with acl_vstring_free.
 */
ACL_API ACL_VSTRING* acl_xml3_build(ACL_XML3* xml, ACL_VSTRING *buf);

/**
 * Dump an xml object to the specified stream. Note that the
 * dumped information is for debugging purposes.
 * @param xml {ACL_XML3*} xml object
 * @param fp {ACL_VSTREAM*} output stream
 */
ACL_API void acl_xml3_dump(ACL_XML3 *xml, ACL_VSTREAM *fp);

/**
 * Dump an xml object to the specified buffer. Note that the dumped information
 * is for debugging purposes.
 * @param xml {ACL_XML3*} xml object
 * @param buf {ACL_VSTRING*} output buffer, no need for user to allocate memory.
 */
ACL_API void acl_xml3_dump2(ACL_XML3 *xml, ACL_VSTRING *buf);

#endif

/***************************************************************************/
/*          The following are low-level interfaces, users can
 *          choose to use them as needed             */
/***************************************************************************/

/*----------------------------- in acl_xml3.c ------------------------------*/

/**
 * Allocate an xml node attribute object.
 * @param node {ACL_XML3_NODE*} xml node
 * @return {ACL_XML3_ATTR*} The newly allocated attribute object
 */
ACL_API ACL_XML3_ATTR *acl_xml3_attr_alloc(ACL_XML3_NODE *node);

/**
 * Allocate an xml node.
 * @param xml {ACL_XML3*} xml object
 * @return {ACL_XML3_NODE*} xml node object
 */
ACL_API ACL_XML3_NODE *acl_xml3_node_alloc(ACL_XML3 *xml);

/**
 * Delete a specific xml node and its child nodes from the xml
 * object, and free the memory occupied by this node and its
 * child nodes. This function will free the memory occupied by
 * the xml node.
 * @param node {ACL_XML3_NODE*} xml node
 * @return {int} The number of deleted nodes
 */
ACL_API int acl_xml3_node_delete(ACL_XML3_NODE *node);

/**
 * Append a sibling node to a specific xml node (the sibling
 * node can also be an xml node with children).
 * @param node1 {ACL_XML3_NODE*} The xml node to which the
 *  sibling node is appended.
 * @param node2 {ACL_XML3_NODE*} The xml node to be appended as a sibling.
 */
ACL_API void acl_xml3_node_append(ACL_XML3_NODE *node1, ACL_XML3_NODE *node2);

/**
 * Add a specific xml node as a child node to another xml node.
 * @param parent {ACL_XML3_NODE*} parent node
 * @param child {ACL_XML3_NODE*} child node
 */
ACL_API void acl_xml3_node_add_child(ACL_XML3_NODE *parent, ACL_XML3_NODE *child);

/**
 * Get the parent node of a specific xml node.
 * @param node {ACL_XML3_NODE*} xml node
 * @return {ACL_XML3_NODE*} The parent node. Returns NULL if
 *  the parent node does not exist.
 */
ACL_API ACL_XML3_NODE *acl_xml3_node_parent(ACL_XML3_NODE *node);

/**
 * Get the next sibling node of a specific xml node.
 * @param node {ACL_XML3_NODE*} xml node
 * @return {ACL_XML3_NODE*} The next sibling node of the xml
 *  node. Returns NULL if it does not exist.
 */
ACL_API ACL_XML3_NODE *acl_xml3_node_next(ACL_XML3_NODE *node);

/**
 * Get the previous sibling node of a specific xml node.
 * @param node {ACL_XML3_NODE*} xml node
 * @return {ACL_XML3_NODE*} The previous sibling node of the
 *  xml node. Returns NULL if it does not exist.
 */
ACL_API ACL_XML3_NODE *acl_xml3_node_prev(ACL_XML3_NODE *node);

#ifdef __cplusplus
}
#endif
#endif
