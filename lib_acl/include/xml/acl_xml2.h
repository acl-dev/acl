#ifndef ACL_XML2_INCLUDE_H
#define ACL_XML2_INCLUDE_H

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

typedef struct ACL_XML2	ACL_XML2;
typedef struct ACL_XML2_NODE	ACL_XML2_NODE;
typedef struct ACL_XML2_ATTR	ACL_XML2_ATTR;

struct ACL_XML2_ATTR {
	ACL_XML2_NODE *node;            /**< parent node */
	char *name;                     /**< attribute name */
	char *value;                    /**< attribute value */

	ssize_t name_size;              /**< attribute name length */
	ssize_t value_size;             /**< attribute value length */

	/* private */
	int   quote;                    /**< If 0 indicates ' or " */
	int   backslash;                /**< escape character \ */
	int   slash;                    /**< whether '/' flag is set */
};

struct ACL_XML2_NODE {
	char *ltag;                     /**< left tag name */
	char *rtag;                     /**< right tag name */
	ssize_t ltag_size;              /**< left tag name length */
	ssize_t rtag_size;              /**< right tag name length */
	const char *id;                 /**< ID identifier, only when xml->id_table
					  contains the node's id is non-empty */
	char *text;                     /**< text content representation */
	ssize_t text_size;              /**< file data length */

	ACL_ARRAY *attr_list;           /**< attribute (ACL_XML2_ATTR) list */
	ACL_XML2_NODE *parent;          /**< parent node */
	ACL_RING children;              /**< child node collection */
	int  depth;                     /**< current node depth */

	/* private */
	ACL_XML2 *xml;                  /**< xml object */
	ACL_RING node;                  /**< current node */
	ACL_XML2_ATTR *curr_attr;       /**< currently being processed attribute */
	int   quote;                    /**< If 0 indicates ' or " */
	int   last_ch;                  /**< last character value
					 *   recorded for the node */
	int   nlt;                      /**< '<' count */
	char  meta[3];                  /**< meta data temporary storage */

	unsigned int flag;
#define	ACL_XML2_F_META_QM	(1 << 0)    /**< '?' flag */
#define	ACL_XML2_F_META_CM	(1 << 1)    /**< '!--' flag */
#define	ACL_XML2_F_META_EM	(1 << 2)    /**< only '!' flag */
#define ACL_XML2_F_SELF_CL	(1 << 3)    /**< self closed flag */
#define	ACL_XML2_F_LEAF		(1 << 4)    /**< leaf node has no child node */
#define	ACL_XML2_F_CDATA	(1 << 5)    /**< CDATA data */

/**< whether it is meta data */
#define	ACL_XML2_F_META		\
	(ACL_XML2_F_META_QM | ACL_XML2_F_META_CM | ACL_XML2_F_META_EM)

#define	ACL_XML2_IS_COMMENT(x)	(((x)->flag & ACL_XML2_F_META_CM))

#define	ACL_XML2_IS_CDATA(x)	(((x)->flag & ACL_XML2_F_CDATA))

	int   status;                   /**< state machine current state */
#define ACL_XML2_S_NXT		0       /**< next node */
#define ACL_XML2_S_LLT		1       /**< left '<' */
#define ACL_XML2_S_LGT		2       /**< left '>' */
#define	ACL_XML2_S_LCH		3       /**< left '<' next character */
#define ACL_XML2_S_LEM		4       /**< left '<' then '!' */
#define ACL_XML2_S_LTAG		5       /**< left tag name */
#define ACL_XML2_S_RLT		6       /**< right '<' */
#define ACL_XML2_S_RGT		7       /**< right '>' */
#define ACL_XML2_S_RTAG		8       /**< right tag name */
#define ACL_XML2_S_ATTR		9       /**< attribute name */
#define ACL_XML2_S_AVAL		10      /**< attribute value */
#define ACL_XML2_S_TXT		11      /**< node text */
#define ACL_XML2_S_MTAG		12      /**< meta data tag */
#define ACL_XML2_S_MTXT		13      /**< meta data text */
#define ACL_XML2_S_MCMT		14      /**< meta data comment */
#define ACL_XML2_S_MEND		15      /**< meta data end */
#define	ACL_XML2_S_CDATA	16      /**< CDATA data */

	/* for acl_iterator, through acl_foreach to iterate
	 * through all child nodes of this node */

	/* Get the head element of the container */
	ACL_XML2_NODE *(*iter_head)(ACL_ITER*, ACL_XML2_NODE*);
	/* Get the next element of the container */
	ACL_XML2_NODE *(*iter_next)(ACL_ITER*, ACL_XML2_NODE*);
	/* Get the tail element of the container */
	ACL_XML2_NODE *(*iter_tail)(ACL_ITER*, ACL_XML2_NODE*);
	/* Get the previous element of the container */
	ACL_XML2_NODE *(*iter_prev)(ACL_ITER*, ACL_XML2_NODE*);
};

struct ACL_XML2 {
	/* public */
					
	int   depth;                    /**< maximum depth */
	int   node_cnt;                 /**< node count, excluding root node */
	int   root_cnt;                 /**< root node count */
	int   attr_cnt;                 /**< attribute count */
	ACL_XML2_NODE *root;            /**< XML root node */

	/* private */
	ACL_FILE_HANDLE fd;		/** File descriptor for the opened file */
	ACL_VSTRING *vbuf;		/**< Buffer for memory-mapped file  */
	ACL_VSTRING *vbuf_inner;	/**< Internal allocated buffer */
	char  dummy[1];

	ACL_HTABLE *id_table;           /**< id identifier hash table */
	ACL_XML2_NODE *curr_node;       /**< current XML node being processed */
	ACL_DBUF_POOL *dbuf;            /**< memory pool object */
	ACL_DBUF_POOL *dbuf_inner;      /**< internal partial memory pool object */
	size_t dbuf_keep;               /**< memory pool reserved length */

	unsigned flag;                  /**< flag bit: ACL_XML2_FLAG_xxx */ 

	/**< whether to allow multiple root nodes in one xml
	 *   document, internal default is allowed */
#define	ACL_XML2_FLAG_MULTI_ROOT	(1 << 0)

	/**< whether to ignore the '/' flag in the node */
#define	ACL_XML2_FLAG_IGNORE_SLASH	(1 << 1)

	/**< when parsing xml data, whether to decode data into xml format  */
#define	ACL_XML2_FLAG_XML_DECODE	(1 << 2)

	/**< when outputting xml data, whether to encode data into xml format */
#define ACL_XML2_FLAG_XML_ENCODE	(1 << 3)

	/* for acl_iterator, through acl_foreach to iterate through all child nodes */

	/* Get the head element of the container */
	ACL_XML2_NODE *(*iter_head)(ACL_ITER*, const ACL_XML2*);
	/* Get the next element of the container */
	ACL_XML2_NODE *(*iter_next)(ACL_ITER*, const ACL_XML2*);
	/* Get the tail element of the container */
	ACL_XML2_NODE *(*iter_tail)(ACL_ITER*, const ACL_XML2*);
	/* Get the previous element of the container */
	ACL_XML2_NODE *(*iter_prev)(ACL_ITER*, const ACL_XML2*);
};

/****************************************************************************/
/*                  Public interface, users can directly use this
 *                  interface                  */
/****************************************************************************/

/*----------------------------- in acl_xml2.c ----------------------------*/

/**
 * Check if xml object is closed, that is, whether all left and
 * right tags match, if xml object's
 * xml node elements are empty, it is also considered closed
 * @param xml {ACL_XML2*} xml object
 * @return {int} 0: No; 1: Yes
 */
ACL_API int acl_xml2_is_closure(ACL_XML2 *xml);

/**
 * Check if xml object has been parsed completely by the specified
 * tag name. This tag name is a child node of xml object's root
 * with the same tag name as a certain xml node, and it is
 * considered that xml object is complete. To ensure the correctness
 * of the check, you should ensure that the root node has only one
 * child, and xml->root has only one child node, otherwise
 * the check will fail
 * @param xml {ACL_XML2*} xml object
 * @param tag {const char*} User-specified tag name,
 *  case-insensitive when matching internally
 * @return {int} 0: No; 1: Yes
 */
ACL_API int acl_xml2_is_complete(ACL_XML2 *xml, const char *tag);

/**
 * Create an xml object.
 * @param vbuf {ACL_VSTRING*} Memory-mapped buffer pointer
 * @return {ACL_XML2*} Newly created xml object
 */
ACL_API ACL_XML2 *acl_xml2_alloc(ACL_VSTRING *vbuf);

/**
 * Create an xml object, all internal memory allocations of this
 * xml object are allocated on this memory pool.
 * @param vbuf {ACL_VSTRING*} Memory-mapped buffer pointer
 * @param dbuf {ACL_DBUF_POOL*} Memory pool object, if NULL, the xml object
 *  will allocate node memory on the heap, otherwise it will
 *  automatically use the xml object's memory pool
 * @return {ACL_XML2*} Newly created xml object
 */
ACL_API ACL_XML2 *acl_xml2_dbuf_alloc(ACL_VSTRING *vbuf, ACL_DBUF_POOL *dbuf);

/**
 * Create an xml object, xml node's data storage memory uses memory-mapped file.
 * @param filepath {const char*} Memory-mapped file's file path
 * @param max_len {size_t} Maximum mapped file size
 * @param init_len {size_t} Mapped memory initial size
 * @param dbuf {ACL_DBUF_POOL*} Memory pool object, if NULL, the xml object
 *  will allocate node memory on the heap, otherwise it will
 *  automatically use the xml object's memory pool
 * @return {ACL_XML2*} Newly created xml object
 */
ACL_API ACL_XML2 *acl_xml2_mmap_file(const char *filepath, size_t max_len,
		size_t init_len, ACL_DBUF_POOL *dbuf);

/**
 * Create an xml object, xml node's data storage memory uses memory-mapped file.
 * @param fd {ACL_FILE_HANDLE} Memory-mapped file's file descriptor,
 *  when the xml object is successfully created,
 *  this descriptor will be closed when the xml object is freed
 * @param max_len {size_t} Maximum mapped file size
 * @param init_len {size_t} Mapped memory initial size
 * @param dbuf {ACL_DBUF_POOL*} Memory pool object, if NULL, the xml object
 *  will allocate node memory on the heap, otherwise it will
 *  automatically use the xml object's memory pool
 * @return {ACL_XML2*} Newly created xml object
 */
ACL_API ACL_XML2 *acl_xml2_mmap_fd(ACL_FILE_HANDLE fd, size_t max_len,
		size_t init_len, ACL_DBUF_POOL *dbuf);

/**
 * Get the current xml object's internally allocated memory space size.
 * @param xml {ACL_XML2*}
 * @return {size_t} Current xml object's internally allocated memory size
 */
ACL_API size_t acl_xml2_space(ACL_XML2 *xml);

/**
 * Clear the internal record of memory size in xml object to 0.
 * @param xml {ACL_XML2*}
 */
ACL_API void acl_xml2_space_clear(ACL_XML2 *xml);

/**
 * Set a certain ACL_XML2_NODE node as the root node of an XML
 * object, so that you can easily iterate through all child nodes
 * of this node (in the iteration process, all nodes are not
 * included in the root node itself). This iteration method lists
 * nodes when iterating through a certain ACL_XML2_NODE node, it
 * can iterate through all child nodes in order
 * @param xml {ACL_XML2*} xml object
 * @param node {ACL_XML2_NODE*} AXL_XML_NODE node
 */
ACL_API void acl_xml2_foreach_init(ACL_XML2 *xml, ACL_XML2_NODE *node);

/**
 * Set whether an xml document allows multiple xml nodes,
 * internal default allows multiple nodes.
 * @param xml {ACL_XML2*} xml object
 * @param on {int} If 0, setting to 0 indicates not to allow,
 *  when stopping allowing multiple xml nodes, if parsing is
 *  incomplete, when encountering a new root node, it will
 *  return the remaining data at that point
 */
ACL_API void acl_xml2_multi_root(ACL_XML2 *xml, int on);

/**
 * Set XML tag closure, whether to ignore the /, e.g.:
 * <test id=111>, <test id=111 />, if the first two do not
 * have / they are considered legal, but only the second
 * writing is legal, this function provides this flexibility,
 * so the first writing will not fail
 * @param xml {ACL_XML2*} xml object
 * @param ignore {int} If 0 indicates the node must have /
 */
ACL_API void acl_xml2_slash(ACL_XML2 *xml, int ignore);

/**
 * Set whether to decode attribute values and text values
 * into xml format when parsing xml data, internal default is
 * not to decode.
 * @param xml {ACL_XML2*}
 * @param on {int} If 0 indicates to enable xml decoding
 */
ACL_API void acl_xml2_decode_enable(ACL_XML2 *xml, int on);

/**
 * Set whether to encode attribute values and text values
 * into xml format when outputting xml data, internal default
 * is not to encode.
 */
ACL_API void acl_xml2_encode_enable(ACL_XML2 *xml, int on);

/**
 * Free an xml object, and also free all xml nodes created by the object.
 * @param xml {ACL_XML2*} xml object
 * @return {int} Number of freed xml nodes
 */
ACL_API int acl_xml2_free(ACL_XML2 *xml);

/**
 * Reset XML object state.
 * @param xml {ACL_XML2*} xml object
 */
ACL_API void acl_xml2_reset(ACL_XML2 *xml);

/*------------------------- in acl_xml2_parse.c ----------------------------*/

/**
 * Parse xml data, and automatically create xml nodes.
 * @param xml {ACL_XML2*} xml object
 * @param data {const char*} A string ending with '\0', which
 *  can be a complete xml string; it can also be an
 *  incomplete xml string, and you can call this function in
 *  a loop, and when the data is complete, it will be parsed
 * @return {const char*} If through acl_xml2_multi_root to
 *  allow multiple xml documents in one xml document, when
 *  encountering a new xml node, the address returned by this
 *  function will be set to '\0'; otherwise returns the
 *  remaining data address which is a non-empty string
 *  Note: You can also use acl_xml2_is_complete to check if parsing is complete
 */
ACL_API const char *acl_xml2_update(ACL_XML2 *xml, const char *data);
#define	acl_xml2_parse	acl_xml2_update

/*------------------------- in acl_xml2_util.c -----------------------------*/

/**
 * Initialize some self-closing tags such as input, br, hr,
 * etc., to form a self-closing tag table, so that
 * acl_xml2_tag_selfclosed can query, to determine whether a
 * tag is a predefined self-closing tag, this function can
 * only be initialized once, or not initialized
 */
ACL_API void acl_xml2_tag_init(void);

/**
 * Add some user-defined self-closing tags.
 * @param tag {const char*} Tag name, note that tag length
 *  should not exceed 254 bytes
 */
ACL_API void acl_xml2_tag_add(const char *tag);

/**
 * Query the self-closing tag table initialized by
 * acl_xml2_tag_init, you can call this function to determine
 * whether a tag is a predefined self-closing tag, if
 * acl_xml2_tag_init has not been called, this function will
 * automatically return 0
 * @parma tag {const char*} Tag name
 * @return {int} 0: indicates no, 1: indicates yes
 */
ACL_API int  acl_xml2_tag_selfclosed(const char *tag);

/**
 * Check if the tag name xml node is a leaf node, leaf nodes
 * have no child nodes.
 * @param tag {const char*} Tag name
 * @return {int} 0: not a leaf node; 1: is a leaf node
 */
ACL_API int  acl_xml2_tag_leaf(const char *tag);

/**
 * Free the dynamic array returned by functions such as
 * acl_xml2_getElementsByTagName, acl_xml2_getElementsByName,
 * acl_xml2_getElementsByAttr, etc. Because the elements in
 * this dynamic array are all pointers to elements in the
 * ACL_XML2 object, when freeing this dynamic array, as long
 * as ACL_XML2 is not freed, the elements originally in the
 * array can still be used.
 * This function only frees the xml node elements
 * @param a {ACL_ARRAY*} Dynamic array pointer
 */
ACL_API void acl_xml2_free_array(ACL_ARRAY *a);

/**
 * Get the first xml node with the same tag name from the xml object.
 * @param xml {ACL_XML2*} xml object
 * @param tag {const char*} Tag name
 * @return {ACL_XML2_NODE*} Returns the found xml node, if NULL
 *  indicates no matching xml node was found
 */
ACL_API ACL_XML2_NODE *acl_xml2_getFirstElementByTagName(
	ACL_XML2 *xml, const char *tag);

/**
 * Get all xml nodes with the same tag name from the xml object collection.
 * @param xml {ACL_XML2*} xml object
 * @param tag {const char*} Tag name
 * @return {ACL_ARRAY*} Returns the found xml node collection,
 *  which is a dynamic array, if NULL indicates no matching
 *  xml node was found, non-empty values need to be freed with
 *  acl_xml2_free_array
 */
ACL_API ACL_ARRAY *acl_xml2_getElementsByTagName(
	ACL_XML2 *xml, const char *tag);

/**
 * Get all xml nodes with the same multi-level tag name from
 * the xml object collection.
 * @param xml {ACL_XML2*} xml object
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
 *  acl_xml2_free_array
 */
ACL_API ACL_ARRAY *acl_xml2_getElementsByTags(ACL_XML2 *xml, const char *tags);

/**
 * Get all xml node elements with the same attribute name attribute value from the xml object collection.
 * @param xml {ACL_XML2*} xml object
 * @param value {const char*} Attribute value with attribute name
 * @return {ACL_ARRAY*} Returns the found xml node collection,
 *  which is a dynamic array, if NULL indicates no matching
 *  xml node was found, non-empty values need to be freed with
 *  acl_xml2_free_array
 */
ACL_API ACL_ARRAY *acl_xml2_getElementsByName(ACL_XML2 *xml, const char *value);

/**
 * Get all xml node elements with the same attribute name and value from the xml object collection.
 * @param xml {ACL_XML2*} xml object
 * @param name {const char*} Attribute name
 * @param value {const char*} Attribute value
 * @return {ACL_ARRAY*} Returns the found xml node collection,
 *  which is a dynamic array, if NULL indicates no matching
 *  xml node was found, non-empty values need to be freed with
 *  acl_xml2_free_array
 */
ACL_API ACL_ARRAY *acl_xml2_getElementsByAttr(ACL_XML2 *xml,
	const char *name, const char *value);

/**
 * Get a certain attribute pointer of an xml node element with the specified
 * id value from the xml object.
 * @param xml {ACL_XML2*} xml object
 * @param id {const char*} id value
 * @return {ACL_XML2_ATTR*} A certain attribute pointer of an xml node, if NULL
 *  indicates no matching attribute was found, this return value does not need to be freed
 */
ACL_API ACL_XML2_ATTR *acl_xml2_getAttrById(ACL_XML2 *xml, const char *id);

/**
 * Get a certain attribute value of an xml node element with the specified id
 * value from the xml object.
 * @param xml {ACL_XML2*} xml object
 * @param id {const char*} id value
 * @return {const char*} A certain attribute value of an xml node, if NULL
 *  indicates no matching attribute was found
 */
ACL_API const char *acl_xml2_getAttrValueById(ACL_XML2 *xml, const char *id);

/**
 * Get an xml node element with the specified id value from the xml object.
 * @param xml {ACL_XML2*} xml object
 * @param id {const char*} id value
 * @return {ACL_XML2_NODE*} xml node element, returns NULL if not found.
 *  Returns an xml node, the return value does not need to be freed.
 */
ACL_API ACL_XML2_NODE *acl_xml2_getElementById(ACL_XML2 *xml, const char *id);

/**
 * Get a node starting with ? ! etc. from the xml object.
 * @param xml {ACL_XML2*} xml object
 * @param tag {const char*} tag name
 * @return {ACL_XML2_NODE*} xml node element, returns NULL if not found.
 *  Returns an xml node, the return value does not need to be freed.
 */
ACL_API ACL_XML2_NODE *acl_xml2_getElementMeta(ACL_XML2 *xml, const char *tag);

/**
 * Get the encoding format of the xml object string.
 * @param xml {ACL_XML2*} xml object
 * @return {const char*} The encoding format string, returns NULL if not set.
 */
ACL_API const char *acl_xml2_getEncoding(ACL_XML2 *xml);

/**
 * Get the content type of the xml object, e.g., text/xsl.
 * @param xml {ACL_XML2*} xml object
 * @return {const char*} Returns NULL if not set.
 */
ACL_API const char *acl_xml2_getType(ACL_XML2 *xml);

/**
 * Get the attribute object of a specific attribute name from an xml node.
 * @param node {ACL_XML2_NODE*} xml node
 * @param name {const char*} attribute name
 * @return {ACL_XML2_ATTR*} attribute object, returns NULL if not found,
 *  this return value does not need to be freed
 */
ACL_API ACL_XML2_ATTR *acl_xml2_getElementAttr(ACL_XML2_NODE *node, const char *name);

/**
 * Get the attribute value of a specific attribute name from an xml node.
 * @param node {ACL_XML2_NODE*} xml node
 * @param name {const char*} attribute name
 * @return {const char*} attribute value, returns NULL if not found.
 */
ACL_API const char *acl_xml2_getElementAttrVal(ACL_XML2_NODE *node, const char *name);

/**
 * Delete a specific attribute from an xml node. If the attribute is an id attribute,
 * it will also be deleted from xml->id_table.
 * @param node {ACL_XML2_NODE*} xml node
 * @param name {const char*} attribute name
 * @return {int} 0 indicates successful deletion, -1 indicates deletion failed
 *  (possibly due to the attribute not existing).
 */
ACL_API int acl_xml2_removeElementAttr(ACL_XML2_NODE *node, const char *name);

/**
 * Add an attribute to an xml node. If the attribute already exists, the new
 * attribute value will replace the old one, and a new attribute object will be returned.
 * @param node {ACL_XML2_NODE*} xml node
 * @param name {const char*} attribute name
 * @param value {const char*} attribute value
 * @return {ACL_XML2_ATTR*} The modified attribute object (possibly the original
 *  one, or a new one), the return value does not need to be freed.
 */
ACL_API ACL_XML2_ATTR *acl_xml2_addElementAttr(ACL_XML2_NODE *node,
        const char *name, const char *value);

/**
 * Create a node with the tag name and text content as an xml node. This function
 * is mainly used when building xml objects.
 * @param xml {ACL_XML2*} xml object, this object should be created by acl_xml2_alloc function.
 * @param tagname {const char*} tag name, must be a non-empty string with length > 0
 * @param text {const char*} node text content, can be empty
 * @return {ACL_XML2_NODE*} The newly created xml node, this function will never
 *  return NULL, and whether the return value is NULL will cause internal automatic
 *  memory allocation.
 */
ACL_API ACL_XML2_NODE *acl_xml2_create_node(ACL_XML2 *xml,
	const char* tagname, const char* text);

/**
 * Create an xml node, using file stream as the node text content, and automatically
 * perform XML encoding processing.
 * @param xml {ACL_XML2*} xml object
 * @param tag {const char*} tag name, can be NULL string
 * @param in {ACL_VSTREAM *} input stream, when NULL, the read data will be used
 *  as xml node text content
 * @param off {size_t} When in is a file stream, specifies the starting position
 *  in the file
 * @param len {size_t} Specifies the maximum length of data that can be read,
 *  when 0, it will read until the stream ends
 * @return {ACL_XML2_NODE*} Returns the newly created xml node, will never return
 *  NULL. Whether the return value is NULL will cause internal automatic memory
 *  allocation.
 */
ACL_API ACL_XML2_NODE *acl_xml2_create_node_with_text_stream(ACL_XML2 *xml,
	const char *tag, ACL_VSTREAM *in, size_t off, size_t len);

/**
 * Add an attribute to an xml node. This function is mainly used when building
 * xml objects.
 * @param node {ACL_XML2_NODE*} Node created by acl_xml2_create_node
 * @param name {const char*} attribute name, must be a non-empty string with
 *  length > 0
 * @param value {const char*} attribute value, can be empty
 * @return {ACL_XML2_ATTR*} xml node attribute object, when the return value is
 *  not NULL, this function will automatically allocate memory internally.
 */
ACL_API ACL_XML2_ATTR *acl_xml2_node_add_attr(ACL_XML2_NODE *node,
	const char *name, const char *value);

/**
 * Add multiple attributes to an xml node. This function is mainly used when
 * building xml objects.
 * @param node {ACL_XML2_NODE*} Node created by acl_xml2_create_node
 * @param ... Multiple attributes, ending with NULL, e.g.:
 *  {name1}, {value1}, {name2}, {value2}, ... NULL
 */
ACL_API void acl_xml2_node_add_attrs(ACL_XML2_NODE *node, ...);

/**
 * Set the text content of an xml node. This function is mainly used when building
 * xml objects. When setting the node's text content, it will replace the original
 * text.
 * @param node {ACL_XML2_NODE*} Node created by acl_xml2_create_node
 * @param text {const char*} text content
 */
ACL_API void acl_xml2_node_set_text(ACL_XML2_NODE *node, const char *text);

/**
 * Append text content to an xml node. This function is mainly used when
 * building xml objects. It will append new text content to the node's
 * text content.
 * @param node {ACL_XML2_NODE*} Node created by acl_xml2_create_node
 * @param text {const char*} text content
 */
ACL_API void acl_xml2_node_add_text(ACL_XML2_NODE *node, const char *text);

/**
 * Set the text content of an xml node from file stream data.
 * @param node {ACL_XML2_NODE*} Node created by acl_xml2_create_node 
 * @param in {ACL_VSTREAM*} input stream
 * @param off {size_t} When in is a file stream, specifies the starting position
 *  in the file
 * @param len {size_t} Length of data to read, when 0, it will read until the
 *  stream ends
 */
ACL_API void acl_xml2_node_set_text_stream(ACL_XML2_NODE *node,
	ACL_VSTREAM *in, size_t off, size_t len);

/**
 * Convert an xml object to a string.
 * @param xml {ACL_XML2*} xml object
 * @return {const char*} The converted string starting address
 */
ACL_API const char *acl_xml2_build(ACL_XML2* xml);

/**
 * Convert an xml object to a string.
 * @param xml {const ACL_XML2*} xml object
 * @param vbuf {ACL_VSTRING*} storage buffer
 * @return {const char*} The converted string starting address
 */
ACL_API const char *acl_xml2_build2(const ACL_XML2* xml, ACL_VSTRING *vbuf);

/**
 * Dump an xml object to the specified stream. Note that the dumped information
 * is for debugging purposes.
 * @param xml {ACL_XML2*} xml object
 * @param fp {ACL_VSTREAM*} output stream
 */
ACL_API void acl_xml2_dump(ACL_XML2 *xml, ACL_VSTREAM *fp);

/**
 * Dump an xml object to the specified buffer. Note that the dumped information
 * is for debugging purposes.
 * @param xml {ACL_XML2*} xml object
 * @param buf {ACL_VSTRING*} output buffer, no need for user to allocate memory.
 */
ACL_API void acl_xml2_dump2(ACL_XML2 *xml, ACL_VSTRING *buf);

/***************************************************************************/
/*          The following are low-level interfaces, users can
 *          choose to use them as needed             */
/***************************************************************************/

/*----------------------------- in acl_xml2.c ------------------------------*/

/**
 * Allocate an xml node attribute object.
 * @param node {ACL_XML2_NODE*} xml node
 * @return {ACL_XML2_ATTR*} The newly allocated attribute object
 */
ACL_API ACL_XML2_ATTR *acl_xml2_attr_alloc(ACL_XML2_NODE *node);

/**
 * Allocate an xml node.
 * @param xml {ACL_XML2*} xml object
 * @return {ACL_XML2_NODE*} xml node object
 */
ACL_API ACL_XML2_NODE *acl_xml2_node_alloc(ACL_XML2 *xml);

/**
 * Delete a specific xml node and its child nodes from the xml object, and free
 * the memory occupied by this node and its child nodes.
 * @param node {ACL_XML2_NODE*} xml node
 * @return {int} The number of deleted nodes
 */
ACL_API int acl_xml2_node_delete(ACL_XML2_NODE *node);

/**
 * Append a sibling node to a specific xml node (the sibling node can also be an
 * xml node with children).
 * @param node1 {ACL_XML2_NODE*} The xml node to which the sibling node is appended.
 * @param node2 {ACL_XML2_NODE*} The xml node to be appended as a sibling.
 */
ACL_API void acl_xml2_node_append(ACL_XML2_NODE *node1, ACL_XML2_NODE *node2);

/**
 * Add a specific xml node as a child node to another xml node.
 * @param parent {ACL_XML2_NODE*} parent node
 * @param child {ACL_XML2_NODE*} child node
 */
ACL_API void acl_xml2_node_add_child(ACL_XML2_NODE *parent, ACL_XML2_NODE *child);

/**
 * Get the parent node of a specific xml node.
 * @param node {ACL_XML2_NODE*} xml node
 * @return {ACL_XML2_NODE*} The parent node. Returns NULL if the parent node does
 *  not exist.
 */
ACL_API ACL_XML2_NODE *acl_xml2_node_parent(ACL_XML2_NODE *node);

/**
 * Get the next sibling node of a specific xml node.
 * @param node {ACL_XML2_NODE*} xml node
 * @return {ACL_XML2_NODE*} The next sibling node of the xml node. Returns NULL if
 *  it does not exist.
 */
ACL_API ACL_XML2_NODE *acl_xml2_node_next(ACL_XML2_NODE *node);

/**
 * Get the previous sibling node of a specific xml node.
 * @param node {ACL_XML2_NODE*} xml node
 * @return {ACL_XML2_NODE*} The previous sibling node of the xml node. Returns NULL
 *  if it does not exist.
 */
ACL_API ACL_XML2_NODE *acl_xml2_node_prev(ACL_XML2_NODE *node);

#ifdef __cplusplus
}
#endif
#endif
