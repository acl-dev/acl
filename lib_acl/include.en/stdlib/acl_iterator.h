#ifndef	ACL_ITERATOR_INCLUDE_H
#define	ACL_ITERATOR_INCLUDE_H

typedef struct ACL_ITER ACL_ITER;

/**
 * Common iterator structure used by ACL data structures.
 */
struct ACL_ITER {
	void *ptr;		/**< Iterator pointer, internal use */
	void *data;		/**< User data pointer */
	int   dlen;		/**< User data length, implementers can
				 *   use or ignore this value */
	const char *key;	/**< For hash tables, this is the key; for
				 *   hash maps, this is the key string */
	int   klen;		/**< For ACL_BINHASH containers, this is the key length */
	int   i;		/**< Current position index in the container */
	int   size;		/**< Total number of elements in the current container */
};

/**
 * Iterate forward through container elements.
 * @param iter {ACL_ITER}
 * @param container {void*} Container address
 * @examples: samples/iterator/
 */
#define	ACL_FOREACH(iter, container)  \
        for ((container)->iter_head(&(iter), (container));  \
             (iter).ptr;  \
             (container)->iter_next(&(iter), (container)))

/**
 * Iterate backward through container elements.
 * @param iter {ACL_ITER}
 * @param container {void*} Container address
 * @examples: samples/iterator/
 */
#define	ACL_FOREACH_REVERSE(iter, container)  \
        for ((container)->iter_tail(&(iter), (container));  \
             (iter).ptr;  \
             (container)->iter_prev(&(iter), (container)))

/**
 * Get the member structure object pointed to by the current iterator.
 * @param iter {ACL_ITER}
 * @param container {void*} Container address
 */
#define	ACL_ITER_INFO(iter, container)  \
	(container)->iter_info(&(iter), (container))

#define	acl_foreach_reverse	ACL_FOREACH_REVERSE
#define	acl_foreach		ACL_FOREACH
#define	acl_iter_info		ACL_ITER_INFO

#endif
