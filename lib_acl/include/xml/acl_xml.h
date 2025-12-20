#ifndef ACL_XML_INCLUDE_H
#define ACL_XML_INCLUDE_H

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

typedef struct ACL_XML	ACL_XML;
typedef struct ACL_XML_NODE	ACL_XML_NODE;
typedef struct ACL_XML_ATTR	ACL_XML_ATTR;

struct ACL_XML_ATTR {
	ACL_XML_NODE *node;             /**< parent node */
	ACL_VSTRING *name;              /**< attribute name */
	ACL_VSTRING *value;             /**< attribute value */

	/* private */
	int   quote;                    /**< If 0 indicates ' or " */
	int   backslash;                /**< escape character \ */
	int   slash;                    /**< whether '/' flag is set */
};

struct ACL_XML_NODE {
	ACL_VSTRING *ltag;              /**< left tag name */
	ACL_VSTRING *rtag;              /**< right tag name */
	const ACL_VSTRING *id;          /**< ID identifier, only when xml->id_table
					 contains the node's id is non-empty */
	ACL_VSTRING *text;              /**< text content representation */
	ACL_ARRAY *attr_list;           /**< attribute (ACL_XML_ATTR) list */
	ACL_XML_NODE *parent;           /**< parent node */
	ACL_RING children;              /**< child node collection */
	int  depth;                     /**< current node depth */

	/* private */
	ACL_XML *xml;                   /**< xml object */
	ACL_RING node;                  /**< current node */
	ACL_XML_ATTR *curr_attr;        /**< currently being processed attribute */
	int   quote;                    /**< If 0 indicates ' or " */
	int   last_ch;                  /**< last character value
					 *   recorded for the node */
	int   nlt;                      /**< '<' count */
	char  meta[3];                  /**< meta data temporary storage */
	unsigned int flag;
#define	ACL_XML_F_META_QM	(1 << 0)    /**< '?' flag */
#define	ACL_XML_F_META_CM	(1 << 1)    /**< '!--' flag */
#define	ACL_XML_F_META_EM	(1 << 2)    /**< only '!' flag */
#define ACL_XML_F_SELF_CL	(1 << 3)    /**< self closed flag */
#define	ACL_XML_F_LEAF		(1 << 4)    /**< leaf node has no child node */
#define	ACL_XML_F_CDATA		(1 << 5)    /**< CDATA data */

/**< whether it is meta data */
#define	ACL_XML_F_META		\
	(ACL_XML_F_META_QM | ACL_XML_F_META_CM | ACL_XML_F_META_EM)

#define	ACL_XML_IS_COMMENT(x)	(((x)->flag & ACL_XML_F_META_CM))

#define	ACL_XML_IS_CDATA(x)	(((x)->flag & ACL_XML_F_CDATA))

	int   status;                   /**< state machine current state */
#define ACL_XML_S_NXT		0       /**< next node */
#define ACL_XML_S_LLT		1       /**< left '<' */
#define ACL_XML_S_LGT		2       /**< left '>' */
#define	ACL_XML_S_LCH		3       /**< left '<' next character */
#define ACL_XML_S_LEM		4       /**< left '<' then '!' */
#define ACL_XML_S_LTAG		5       /**< left tag name */
#define ACL_XML_S_RLT		6       /**< right '<' */
#define ACL_XML_S_RGT		7       /**< right '>' */
#define ACL_XML_S_RTAG		8       /**< right tag name */
#define ACL_XML_S_ATTR		9       /**< attribute name */
#define ACL_XML_S_AVAL		10      /**< attribute value */
#define ACL_XML_S_TXT		11      /**< node text */
#define ACL_XML_S_MTAG		12      /**< meta data tag */
#define ACL_XML_S_MTXT		13      /**< meta data text */
#define ACL_XML_S_MCMT		14      /**< meta data comment */
#define ACL_XML_S_MEND		15      /**< meta data end */
#define	ACL_XML_S_CDATA		16      /**< CDATA data */

	/* public: for acl_iterator, through acl_foreach to
	 * iterate through all child nodes of this node */

	/* Get the head element of the container */
	ACL_XML_NODE *(*iter_head)(ACL_ITER*, ACL_XML_NODE*);
	/* Get the next element of the container */
	ACL_XML_NODE *(*iter_next)(ACL_ITER*, ACL_XML_NODE*);
	/* Get the tail element of the container */
	ACL_XML_NODE *(*iter_tail)(ACL_ITER*, ACL_XML_NODE*);
	/* Get the previous element of the container */
	ACL_XML_NODE *(*iter_prev)(ACL_ITER*, ACL_XML_NODE*);
};

struct ACL_XML {
	/* public */
	int   depth;                    /**< maximum depth */
	int   node_cnt;                 /**< node count, excluding root node */
	int   root_cnt;                 /**< root node count */
	int   attr_cnt;                 /**< attribute count */
	ACL_XML_NODE *root;             /**< XML root node */

	/* private */
	ACL_HTABLE *id_table;           /**< id identifier hash table */
	ACL_XML_NODE *curr_node;        /**< current XML node being processed */
	ACL_DBUF_POOL *dbuf;            /**< memory pool object */
	ACL_DBUF_POOL *dbuf_inner;      /**< internal partial memory pool object */
	size_t dbuf_keep;               /**< memory pool reserved length */
	size_t space;                   /**< memory space allocated
					 *   when parsing xml data */

	unsigned flag;                  /**< flag bit: ACL_XML_FLAG_xxx */ 

	/**< whether to allow multiple root nodes in one xml
	 *   document, internal default is allowed */
#define	ACL_XML_FLAG_MULTI_ROOT		(1 << 0)

	/**< whether to ignore the '/' flag in the node */
#define	ACL_XML_FLAG_IGNORE_SLASH	(1 << 1)

	/**< whether to decode text content into xml format  */
#define	ACL_XML_FLAG_XML_DECODE		(1 << 2)

	/**< when outputting xml data, whether to encode data into xml format */
#define ACL_XML_FLAG_XML_ENCODE		(1 << 3)

	ACL_VSTRING *decode_buf;        /**< non-empty when decoding xml data */

	/* public: for acl_iterator, through acl_foreach to iterate through all child nodes */

	/* Get the head element of the container */
	ACL_XML_NODE *(*iter_head)(ACL_ITER*, ACL_XML*);
	/* Get the next element of the container */
	ACL_XML_NODE *(*iter_next)(ACL_ITER*, ACL_XML*);
	/* Get the tail element of the container */
	ACL_XML_NODE *(*iter_tail)(ACL_ITER*, ACL_XML*);
	/* Get the previous element of the container */
	ACL_XML_NODE *(*iter_prev)(ACL_ITER*, ACL_XML*);
};

/***************************************************************************/
/*                  Public interface, users can directly use this
 *                  interface                 */
/***************************************************************************/

/*----------------------------- in acl_xml.c ------------------------------*/

/**
 * Check if xml object is closed, that is, whether all left and
 * right tags match, if xml object's
 * xml node elements are empty, it is also considered closed
 * @param xml {ACL_XML*} xml object
 * @return {int} 0: No; 1: Yes
 */
ACL_API int acl_xml_is_closure(ACL_XML *xml);

/**
 * Check if xml object has been parsed completely by the specified
 * tag name. This tag name is a child node of xml object's root
 * with the same tag name as a certain xml node, and it is
 * considered that xml object is complete. To ensure the correctness
 * of the check, you should ensure that the root node has only one
 * child, and xml->root has only one child node, otherwise
 * the check will fail
 * @param xml {ACL_XML*} xml object
 * @param tag {const char*} User-specified tag name,
 *  case-insensitive when matching internally
 * @return {int} 0: No; 1: Yes
 */
ACL_API int acl_xml_is_complete(ACL_XML *xml, const char *tag);

/**
 * Create an xml object.
 * @return {ACL_XML*} Newly created xml object
 */
ACL_API ACL_XML *acl_xml_alloc(void);

/**
 * Create an xml object, all internal memory allocations of this
 * xml object are allocated on this memory pool.
 * @param dbuf {ACL_DBUF_POOL*} Memory pool object, if NULL, the xml object
 *  will allocate node memory on the heap, otherwise it will
 *  automatically use the xml object's memory pool
 * @return {ACL_XML*} Newly created xml object
 */
ACL_API ACL_XML *acl_xml_dbuf_alloc(ACL_DBUF_POOL *dbuf);

/**
 * Get the current xml object's internally allocated memory space size.
 * @param xml {ACL_XML*}
 * @return {size_t} Current xml object's internally allocated memory size
 */
ACL_API size_t acl_xml_space(ACL_XML *xml);

/**
 * Clear the internal record of memory size in xml object to 0.
 * @param xml {ACL_XML*}
 */
ACL_API void acl_xml_space_clear(ACL_XML *xml);

/**
 * Set a certain ACL_XML_NODE node as the root node of an XML
 * object, so that you can easily iterate through all child nodes
 * of this node (in the iteration process, all nodes are not
 * included in the root node itself). This iteration method lists
 * nodes when iterating through a certain ACL_XML_NODE node, it
 * can iterate through all child nodes in order
 * @param xml {ACL_XML*} xml object
 * @param node {ACL_XML_NODE*} AXL_XML_NODE node
 */
ACL_API void acl_xml_foreach_init(ACL_XML *xml, ACL_XML_NODE *node);

/**
 * Set whether an xml document allows multiple xml nodes, internal
 * default allows multiple nodes.
 * @param xml {ACL_XML*} xml object
 * @param on {int} If 0, setting to 0 indicates not to allow, when
 *  stopping allowing multiple xml nodes, if parsing is incomplete,
 *  when encountering a new root node, it will return the remaining
 *  data at that point
 */
ACL_API void acl_xml_multi_root(ACL_XML *xml, int on);

/**
 * Set XML tag closure, whether to ignore the /, e.g.:
 * <test id=111>, <test id=111 />, if the first two do not
 * have / they are considered legal, but only the second
 * writing is legal, this function provides this flexibility,
 * so the first writing will not fail
 * @param xml {ACL_XML*} xml object
 * @param ignore {int} If 0 indicates the node must have /
 */
ACL_API void acl_xml_slash(ACL_XML *xml, int ignore);

/**
 * Set whether to decode attribute values and text values
 * into xml format when parsing xml data, internal default is
 * not to decode.
 * @param xml {ACL_XML*}
 * @param on {int} If 0 indicates to enable xml decoding
 */
ACL_API void acl_xml_decode_enable(ACL_XML *xml, int on);

/**
 * Set whether to encode attribute values and text values
 * into xml format when outputting xml data, internal default
 * is not to encode.
 */
ACL_API void acl_xml_encode_enable(ACL_XML *xml, int on);

/**
 * Free an xml object, and also free all xml nodes created by the object.
 * @param xml {ACL_XML*} xml object
 * @return {int} Number of freed xml nodes
 */
ACL_API int acl_xml_free(ACL_XML *xml);

/**
 * Reset XML object state.
 * @param xml {ACL_XML*} xml object
 */
ACL_API void acl_xml_reset(ACL_XML *xml);

/*------------------------- in acl_xml_parse.c ----------------------------*/

/**
 * Parse xml data, and automatically create xml nodes.
 * @param xml {ACL_XML*} xml object
 * @param data {const char*} A string ending with '\0', which
 *  can be a complete xml string; it can also be an
 *  incomplete xml string, and you can call this function in
 *  a loop, and when the data is complete, it will be parsed
 * @return {const char*} If through acl_xml_multi_root to allow
 *  multiple xml documents in one xml document, when
 *  encountering a new xml node, the address returned by this
 *  function will be set to '\0'; otherwise returns the
 *  remaining data address
 *  which is a non-empty string
 *  Note: You can also use acl_xml_is_complete to check if parsing is complete
 */
ACL_API const char *acl_xml_update(ACL_XML *xml, const char *data);
#define	acl_xml_parse	acl_xml_update

/*------------------------- in acl_xml_util.c -----------------------------*/

/**
 * Initialize some self-closing tags such as input, br, hr,
 * etc., to form a self-closing tag table, so that
 * acl_xml_tag_selfclosed can query, to determine whether a
 * tag is a predefined self-closing tag, this function can
 * only be initialized once, or not initialized
 */
ACL_API void acl_xml_tag_init(void);

/**
 * Add some user-defined self-closing tags.
 * @param tag {const char*} Tag name, note that tag length
 *  should not exceed 254 bytes
 */
ACL_API void acl_xml_tag_add(const char *tag);

/**
 * Query the self-closing tag table initialized by
 * acl_xml_tag_init, you can call this function to determine
 * whether a tag is a predefined self-closing tag, if
 * acl_xml_tag_init has not been called, this function will
 * automatically return 0
 * @parma tag {const char*} Tag name
 * @return {int} 0: indicates no, 1: indicates yes
 */
ACL_API int  acl_xml_tag_selfclosed(const char *tag);

/**
 * Check if the tag name xml node is a leaf node, leaf nodes
 * have no child nodes.
 * @param tag {const char*} Tag name
 * @return {int} 0: not a leaf node; 1: is a leaf node
 */
ACL_API int  acl_xml_tag_leaf(const char *tag);

/**
 * Free the dynamic array returned by functions such as
 * acl_xml_getElementsByTagName, acl_xml_getElementsByName,
 * acl_xml_getElementsByAttr, etc. Because the elements in
 * this dynamic array are all pointers to elements in the
 * ACL_XML object, when freeing this dynamic array, as long as
 * ACL_XML is not freed, the elements originally in the array
 * can still be used.
 * This function only frees the xml node elements
 * @param a {ACL_ARRAY*} Dynamic array pointer
 */
ACL_API void acl_xml_free_array(ACL_ARRAY *a);

/**
 * Get the first xml node with the same tag name from the xml object.
 * @param xml {ACL_XML*} xml object
 * @param tag {const char*} Tag name
 * @return {ACL_XML_NODE*} Returns the found xml node, if NULL
 *  indicates no matching xml node was found
 */
ACL_API ACL_XML_NODE *acl_xml_getFirstElementByTagName(
	ACL_XML *xml, const char *tag);

/**
 * Get all xml nodes with the same tag name from the xml object collection.
 * @param xml {ACL_XML*} xml object
 * @param tag {const char*} Tag name
 * @return {ACL_ARRAY*} Returns the found xml node collection,
 *  which is a dynamic array, if NULL indicates no matching
 *  xml node was found, non-empty values need to be freed with
 *  acl_xml_free_array
 */
ACL_API ACL_ARRAY *acl_xml_getElementsByTagName(
	ACL_XML *xml, const char *tag);

/**
 * Get all xml nodes with the same multi-level tag name from
 * the xml object collection.
 * @param xml {ACL_XML*} xml object
 * @param tags {const char*} Multi-level tag name, separated
 *  by '/' to form a tag path, for example xml data: <root>
 *  <first> <second> <third name="test1"> text1 </third>
 *  </second> </first> <root> <first> <second> <third
 *  name="test2"> text2 </third> </second> </first> <root>
 *  <first> <second> <third name="test3"> text3 </third>
 *  </second> </first> You can use the multi-level tag path
 *  root/first/second/third to find all matching nodes at
 *  once
 * @return {ACL_ARRAY*} Returns the found xml node collection,
 *  which is a dynamic array, if NULL indicates no matching
 *  xml node was found, non-empty values need to be freed with
 *  acl_xml_free_array
 */
ACL_API ACL_ARRAY *acl_xml_getElementsByTags(ACL_XML *xml, const char *tags);

/**
 * Get all xml node elements with the same attribute name attribute value from the xml object collection.
 * @param xml {ACL_XML*} xml object
 * @param value {const char*} Attribute value with attribute name
 * @return {ACL_ARRAY*} Returns the found xml node collection,
 *  which is a dynamic array, if NULL indicates no matching
 *  xml node was found, non-empty values need to be freed with
 *  acl_xml_free_array
 */
ACL_API ACL_ARRAY *acl_xml_getElementsByName(ACL_XML *xml, const char *value);

/**
 * Get all xml node elements with the same attribute name and value from the xml object collection.
 * @param xml {ACL_XML*} xml object
 * @param name {const char*} Attribute name
 * @param value {const char*} Attribute value
 * @return {ACL_ARRAY*} Returns the found xml node collection,
 *  which is a dynamic array, if NULL indicates no matching
 *  xml node was found, non-empty values need to be freed with
 *  acl_xml_free_array
 */
ACL_API ACL_ARRAY *acl_xml_getElementsByAttr(ACL_XML *xml,
	const char *name, const char *value);

/**
 * Get a certain attribute pointer of an xml node element with the specified id
 * value from the xml object.
 * @param xml {ACL_XML*} xml object
 * @param id {const char*} id value
 * @return {ACL_XML_ATTR*} A certain attribute pointer of an xml node, if NULL indicates
 *  no matching attribute was found, this return value does not need to be freed
 */
ACL_API ACL_XML_ATTR *acl_xml_getAttrById(ACL_XML *xml, const char *id);

/**
 * Get a certain attribute value of an xml node element with the specified id
 * value from the xml object.
 * @param xml {ACL_XML*} xml object
 * @param id {const char*} id value
 * @return {const char*} A certain attribute value of an xml node, if NULL
 *  indicates no matching attribute was found
 */
ACL_API const char *acl_xml_getAttrValueById(ACL_XML *xml, const char *id);

/**
 * Get an xml node element with the specified id value from the xml object.
 * @param xml {ACL_XML*} xml object
 * @param id {const char*} id value
 * @return {ACL_XML_NODE*} xml node element, if NULL indicates no matching
 *  xml node was found, this return value does not need to be freed
 */
ACL_API ACL_XML_NODE *acl_xml_getElementById(ACL_XML *xml, const char *id);

/**
 * Get nodes starting with ? ! etc. from the xml object.
 * @param xml {ACL_XML*} xml object
 * @param tag {const char*} Tag name
 * @return {ACL_XML_NODE*} xml node element, if NULL indicates no matching
 *  xml node was found, this return value does not need to be freed
 */
ACL_API ACL_XML_NODE *acl_xml_getElementMeta(ACL_XML *xml, const char *tag);

/**
 * Get xml document encoding format.
 * @param xml {ACL_XML*} xml object
 * @return {const char*} Returns the encoding format string, if NULL indicates
 *  no encoding was found
 */
ACL_API const char *acl_xml_getEncoding(ACL_XML *xml);

/**
 * Get xml data type, e.g.: text/xsl
 * @param xml {ACL_XML*} xml object
 * @return {const char*} If NULL indicates no type was found
 */
ACL_API const char *acl_xml_getType(ACL_XML *xml);

/**
 * Get a certain attribute pointer from an xml node.
 * @param node {ACL_XML_NODE*} xml node
 * @param name {const char*} Attribute name
 * @return {ACL_XML_ATTR*} Attribute pointer, if empty indicates not found,
 *  this return value does not need to be freed
 */
ACL_API ACL_XML_ATTR *acl_xml_getElementAttr(ACL_XML_NODE *node, const char *name);

/**
 * Get a certain attribute value from an xml node.
 * @param node {ACL_XML_NODE*} xml node
 * @param name {const char*} Attribute name
 * @return {const char*} Attribute value, if empty indicates not found
 */
ACL_API const char *acl_xml_getElementAttrVal(ACL_XML_NODE *node, const char *name);

/**
 * Remove a certain attribute pointer from an xml node, if the attribute name
 * is id, it will also remove it from xml->id_table.
 * @param node {ACL_XML_NODE*} xml node
 * @param name {const char*} Attribute name
 * @return {int} 0 indicates deletion successful, -1: indicates deletion failed
 *  (possibly the attribute does not exist)
 */
ACL_API int acl_xml_removeElementAttr(ACL_XML_NODE *node, const char *name);

/**
 * Add an attribute to an xml node, if the attribute already exists, replace
 * the old attribute value with the new attribute value, otherwise create
 * a new attribute pointer
 * @param node {ACL_XML_NODE*} xml node
 * @param name {const char*} Attribute name
 * @param value {const char*} Attribute value
 * @return {ACL_XML_ATTR*} Returns the attribute pointer (may be the original
 *  one, or may be a new one), this return value does not need to be freed
 */
ACL_API ACL_XML_ATTR *acl_xml_addElementAttr(ACL_XML_NODE *node,
        const char *name, const char *value);

/**
 * Create a tag name node with text content as a new xml node, this function
 * needs to be used when building xml data.
 * @param xml {ACL_XML*} xml object, this object should be created by acl_xml_alloc
 * @param tagname {const char*} Tag name, non-empty string, length must be > 0
 * @param text {const char*} Node's text content, can be empty
 * @return {ACL_XML_NODE*} Newly created xml node, this return value is always
 *  non-empty, regardless of whether the return value is NULL will cause
 *  internal automatic memory allocation.
 */
ACL_API ACL_XML_NODE *acl_xml_create_node(ACL_XML *xml,
	const char* tagname, const char* text);

/**
 * Create xml node, use file stream as node's text content, and automatically
 * perform XML encoding processing.
 * @param xml {ACL_XML*} xml object
 * @param tag {const char*} Tag name, can be NULL string
 * @param in {ACL_VSTREAM *} Input stream, if NULL, the read data will be used
 *  as xml node's text content
 * @param off {size_t} When in is a file stream, specifies the starting
 *  position in the file
 * @param len {size_t} Specifies the maximum length of data that can be read,
 *  when 0, it will read until the end of the stream
 * @return {ACL_XML_NODE*} Returns the newly created xml node, always returns
 *  non-NULL, regardless of whether the return value is NULL will cause
 *  internal automatic errors
 */
ACL_API ACL_XML_NODE *acl_xml_create_node_with_text_stream(ACL_XML *xml,
	const char *tag, ACL_VSTREAM *in, size_t off, size_t len);

/**
 * Add an attribute to an xml node, this function needs to be used
 * when building xml data.
 * @param node {ACL_XML_NODE*} Node created by acl_xml_create_node
 * @param name {const char*} Attribute name, non-empty string, string length must be > 0
 * @param value {const char*} Attribute value, can be empty
 * @return {ACL_XML_ATTR*} xml node's attribute pointer, when the return value
 *  is NULL, this function will automatically cause errors internally
 */
ACL_API ACL_XML_ATTR *acl_xml_node_add_attr(ACL_XML_NODE *node,
	const char *name, const char *value);

/**
 * Add multiple attributes to an xml node, this function needs to be used
 * when building xml data.
 * @param node {ACL_XML_NODE*} Node created by acl_xml_create_node
 * @param ... Multiple attributes, ending with NULL, e.g.:
 *  {name1}, {value1}, {name2}, {value2}, ... NULL
 */
ACL_API void acl_xml_node_add_attrs(ACL_XML_NODE *node, ...);

/**
 * Set an xml node's text content, this function needs to be used when building
 * xml data. When setting text content before this node,
 * it will replace the original text
 * @param node {ACL_XML_NODE*} Node created by acl_xml_create_node
 * @param text {const char*} Text content
 */
ACL_API void acl_xml_node_set_text(ACL_XML_NODE *node, const char *text);

/**
 * Append text content to an xml node, this function needs to be used when
 * building xml data. Append new text content to this node's text content.
 * @param node {ACL_XML_NODE*} Node created by acl_xml_create_node
 * @param text {const char*} Text content
 */
ACL_API void acl_xml_node_add_text(ACL_XML_NODE *node, const char *text);

/**
 * Set file stream data as an xml node's text content.
 * @param node {ACL_XML_NODE*} Node created by acl_xml_create_node 
 * @param fp {ACL_VSTREAM*} Input stream pointer
 * @param off {size_t} When in is a file stream, specifies the starting position
 *  in the file
 * @param len {size_t} Length of data to be read, when 0, it will read until
 *  the end of the stream
 */
ACL_API void acl_xml_node_set_text_stream(ACL_XML_NODE *node,
	ACL_VSTREAM *fp, size_t off, size_t len);

/**
 * Convert xml object to string format.
 * @param xml {ACL_XML*} xml object
 * @param buf {ACL_VSTRING*} Buffer to store the result. If this parameter
 *  is NULL, the internal will automatically allocate a buffer. If the user
 *  needs to free it, it is a non-empty buffer allocated internally, otherwise
 *  it will directly use the storage space provided by the user
 * @return {ACL_VSTRING*} Buffer to store the converted string format of the
 *  xml object. This return value is always non-empty.
 *  Users can determine whether the content is empty through ACL_VSTRING_LEN(x).
 *  The returned ACL_VSTRING pointer is allocated internally by this function.
 *  If the user needs to free it, call acl_vstring_free to free it
 */
ACL_API ACL_VSTRING* acl_xml_build(ACL_XML* xml, ACL_VSTRING *buf);

/**
 * Convert xml object to the specified stream, note that the converted
 * information is for debugging purposes only.
 * @param xml {ACL_XML*} xml object
 * @param fp {ACL_VSTREAM*} Output stream
 */
ACL_API void acl_xml_dump(ACL_XML *xml, ACL_VSTREAM *fp);

/**
 * Convert xml object to the specified string buffer, note that the converted
 * information is for debugging purposes only.
 * @param xml {ACL_XML*} xml object
 * @param buf {ACL_VSTRING*} Buffer, user needs to allocate space themselves
 */
ACL_API void acl_xml_dump2(ACL_XML *xml, ACL_VSTRING *buf);

/***************************************************************************/
/*  The following are low-level interfaces, users can extend new
 *  interfaces as needed                                */
/***************************************************************************/

/*----------------------------- in acl_xml.c ------------------------------*/

/**
 * Allocate xml node attribute.
 * @param node {ACL_XML_NODE*} xml node
 * @return {ACL_XML_ATTR*} Newly created node attribute
 */
ACL_API ACL_XML_ATTR *acl_xml_attr_alloc(ACL_XML_NODE *node);

/**
 * Allocate an xml node.
 * @param xml {ACL_XML*} xml object
 * @return {ACL_XML_NODE*} xml node pointer
 */
ACL_API ACL_XML_NODE *acl_xml_node_alloc(ACL_XML *xml);

/**
 * Delete an xml node and its child nodes from the xml object, and free the node
 * and child nodes' memory space.
 * The memory pool function will free the memory space of the xml node.
 * @param node {ACL_XML_NODE*} xml node
 * @return {int} Number of deleted nodes
 */
ACL_API int acl_xml_node_delete(ACL_XML_NODE *node);

/**
 * Append a sibling node to an xml node (the sibling node must be a detached xml node)
 * @param node1 {ACL_XML_NODE*} The node to append to
 * @param node2 {ACL_XML_NODE*} The sibling xml node to append
 */
ACL_API void acl_xml_node_append(ACL_XML_NODE *node1, ACL_XML_NODE *node2);

/**
 * Add an xml node as a child node to an xml node.
 * @param parent {ACL_XML_NODE*} Parent node
 * @param child {ACL_XML_NODE*} Child node
 */
ACL_API void acl_xml_node_add_child(ACL_XML_NODE *parent, ACL_XML_NODE *child);

/**
 * Get the parent node of an xml node.
 * @param node {ACL_XML_NODE*} xml node
 * @return {ACL_XML_NODE*} Parent node, if NULL indicates its parent node does not exist
 */
ACL_API ACL_XML_NODE *acl_xml_node_parent(ACL_XML_NODE *node);

/**
 * Get the next sibling node of an xml node.
 * @param node {ACL_XML_NODE*} xml node
 * @return {ACL_XML_NODE*} The next sibling node of the xml node, if NULL
 *  indicates it does not exist
 */
ACL_API ACL_XML_NODE *acl_xml_node_next(ACL_XML_NODE *node);

/**
 * Get the previous sibling node of an xml node.
 * @param node {ACL_XML_NODE*} xml node
 * @return {ACL_XML_NODE*} The previous sibling node of the xml node, if NULL
 *  indicates it does not exist
 */
ACL_API ACL_XML_NODE *acl_xml_node_prev(ACL_XML_NODE *node);

#ifdef __cplusplus
}
#endif
#endif
