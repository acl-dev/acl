/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	AVL_H
#define	AVL_H

/*
 * This is a private header file.  Applications should not directly include
 * this file.
 */

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "avl_impl.h"

/*
 * This is a generic implemenatation of AVL trees for use in the Solaris kernel.
 * The interfaces provide an efficient way of implementing an ordered set of
 * data structures.
 *
 * AVL trees provide an alternative to using an ordered linked list. Using AVL
 * trees will usually be faster, however they requires more storage. An ordered
 * linked list in general requires 2 pointers in each data structure. The
 * AVL tree implementation uses 3 pointers. The following chart gives the
 * approximate performance of operations with the different approaches:
 *
 *	Operation	 Link List	AVL tree
 *	---------	 --------	--------
 *	lookup		   O(n)		O(log(n))
 *
 *	insert 1 node	 constant	constant
 *
 *	delete 1 node	 constant	between constant and O(log(n))
 *
 *	delete all nodes   O(n)		O(n)
 *
 *	visit the next
 *	or prev node	 constant	between constant and O(log(n))
 *
 *
 * The data structure nodes are anchored at an "fiber_avl_tree_t" (the equivalent
 * of a list header) and the individual nodes will have a field of
 * type "fiber_avl_node_t" (corresponding to list pointers).
 *
 * The type "fiber_avl_index_t" is used to indicate a position in the list for
 * certain calls.
 *
 * The usage scenario is generally:
 *
 * 1. Create the list/tree with: fiber_avl_create()
 *
 * followed by any mixture of:
 *
 * 2a. Insert nodes with: fiber_avl_add(), or fiber_avl_find() and fiber_avl_insert()
 *
 * 2b. Visited elements with:
 *	 fiber_avl_first() - returns the lowest valued node
 *	 fiber_avl_last() - returns the highest valued node
 *	 AVL_NEXT() - given a node go to next higher one
 *	 AVL_PREV() - given a node go to previous lower one
 *
 * 2c.  Find the node with the closest value either less than or greater
 *	than a given value with fiber_avl_nearest().
 *
 * 2d. Remove individual nodes from the list/tree with fiber_avl_remove().
 *
 * and finally when the list is being destroyed
 *
 * 3. Use fiber_avl_destroy_nodes() to quickly process/free up any remaining nodes.
 *    Note that once you use fiber_avl_destroy_nodes(), you can no longer
 *    use any routine except fiber_avl_destroy_nodes() and avl_destoy().
 *
 * 4. Use fiber_avl_destroy() to destroy the AVL tree itself.
 *
 * Any locking for multiple thread access is up to the user to provide, just
 * as is needed for any linked list implementation.
 */

/*
 * Type used for the root of the AVL tree.
 */
typedef struct fiber_avl_tree fiber_avl_tree_t;

/*
 * The data nodes in the AVL tree must have a field of this type.
 */
typedef struct fiber_avl_node fiber_avl_node_t;

/*
 * An opaque type used to locate a position in the tree where a node
 * would be inserted.
 */
#ifdef	MS_VC6
typedef unsigned int uintptr_t;
typedef uintptr_t fiber_avl_index_t;
#elif	defined(BORLAND_CB)
typedef unsigned int uintptr_t;
typedef uintptr_t fiber_avl_index_t;
#else
typedef uintptr_t fiber_avl_index_t;
#endif

/*
 * Direction constants used for fiber_avl_nearest().
 */
#define	AVL_BEFORE	(0)
#define	AVL_AFTER	(1)


/*
 * Prototypes
 *
 * Where not otherwise mentioned, "void *" arguments are a pointer to the
 * user data structure which must contain a field of type fiber_avl_node_t.
 *
 * Also assume the user data structures looks like:
 *	stuct my_type {
 *		...
 *		fiber_avl_node_t	my_link;
 *		...
 *	};
 */

/*
 * Initialize an AVL tree. Arguments are:
 *
 * tree   - the tree to be initialized
 * compar - function to compare two nodes, it must return exactly: -1, 0, or +1
 *          -1 for <, 0 for ==, and +1 for >
 * size   - the value of sizeof(struct my_type)
 * offset - the value of OFFSETOF(struct my_type, my_link)
 */
void fiber_avl_create(fiber_avl_tree_t *tree,
	int (*compar) (const void *, const void *), size_t size, size_t offset);


/*
 * Find a node with a matching value in the tree. Returns the matching node
 * found. If not found, it returns NULL and then if "where" is not NULL it sets
 * "where" for use with fiber_avl_insert() or fiber_avl_nearest().
 *
 * node   - node that has the value being looked for
 * where  - position for use with fiber_avl_nearest() or fiber_avl_insert(), may be NULL
 */
void *fiber_avl_find(fiber_avl_tree_t *tree, void *node, fiber_avl_index_t *where);

/*
 * Insert a node into the tree.
 *
 * node   - the node to insert
 * where  - position as returned from fiber_avl_find()
 */
void fiber_avl_insert(fiber_avl_tree_t *tree, void *node, fiber_avl_index_t where);

/*
 * Insert "new_data" in "tree" in the given "direction" either after
 * or before the data "here".
 *
 * This might be usefull for avl clients caching recently accessed
 * data to avoid doing fiber_avl_find() again for insertion.
 *
 * new_data	- new data to insert
 * here		- existing node in "tree"
 * direction	- either AVL_AFTER or AVL_BEFORE the data "here".
 */
void fiber_avl_insert_here(fiber_avl_tree_t *tree, void *new_data, void *here,
    int direction);


/*
 * Return the first or last valued node in the tree. Will return NULL
 * if the tree is empty.
 *
 */
void *fiber_avl_first(fiber_avl_tree_t *tree);
void *fiber_avl_last(fiber_avl_tree_t *tree);


/*
 * Return the next or previous valued node in the tree.
 * AVL_NEXT() will return NULL if at the last node.
 * AVL_PREV() will return NULL if at the first node.
 *
 * node   - the node from which the next or previous node is found
 */
#define	AVL_NEXT(tree, node)	fiber_avl_walk(tree, node, AVL_AFTER)
#define	AVL_PREV(tree, node)	fiber_avl_walk(tree, node, AVL_BEFORE)


/*
 * Find the node with the nearest value either greater or less than
 * the value from a previous fiber_avl_find(). Returns the node or NULL if
 * there isn't a matching one.
 *
 * where     - position as returned from fiber_avl_find()
 * direction - either AVL_BEFORE or AVL_AFTER
 *
 * EXAMPLE get the greatest node that is less than a given value:
 *
 *	fiber_avl_tree_t *tree;
 *	struct my_data look_for_value = {....};
 *	struct my_data *node;
 *	struct my_data *less;
 *	fiber_avl_index_t where;
 *
 *	node = fiber_avl_find(tree, &look_for_value, &where);
 *	if (node != NULL)
 *		less = AVL_PREV(tree, node);
 *	else
 *		less = fiber_avl_nearest(tree, where, AVL_BEFORE);
 */
void *fiber_avl_nearest(fiber_avl_tree_t *tree, fiber_avl_index_t where, int direction);


/*
 * Add a single node to the tree.
 * The node must not be in the tree, and it must not
 * compare equal to any other node already in the tree.
 *
 * node   - the node to add
 */
void fiber_avl_add(fiber_avl_tree_t *tree, void *node);


/*
 * Remove a single node from the tree.  The node must be in the tree.
 *
 * node   - the node to remove
 */
void fiber_avl_remove(fiber_avl_tree_t *tree, void *node);

/*
 * Reinsert a node only if its order has changed relative to its nearest
 * neighbors. To optimize performance fiber_avl_update_lt() checks only the previous
 * node and fiber_avl_update_gt() checks only the next node. Use fiber_avl_update_lt() and
 * fiber_avl_update_gt() only if you know the direction in which the order of the
 * node may change.
 */
boolean_t fiber_avl_update(fiber_avl_tree_t *, void *);
boolean_t fiber_avl_update_lt(fiber_avl_tree_t *, void *);
boolean_t fiber_avl_update_gt(fiber_avl_tree_t *, void *);

/*
 * Return the number of nodes in the tree
 */
ulong_t fiber_avl_numnodes(fiber_avl_tree_t *tree);

/*
 * Return B_TRUE if there are zero nodes in the tree, B_FALSE otherwise.
 */
boolean_t fiber_avl_is_empty(fiber_avl_tree_t *tree);

/*
 * Used to destroy any remaining nodes in a tree. The cookie argument should
 * be initialized to NULL before the first call. Returns a node that has been
 * removed from the tree and may be free()'d. Returns NULL when the tree is
 * empty.
 *
 * Once you call fiber_avl_destroy_nodes(), you can only continuing calling it and
 * finally fiber_avl_destroy(). No other AVL routines will be valid.
 *
 * cookie - a "void *" used to save state between calls to fiber_avl_destroy_nodes()
 *
 * EXAMPLE:
 *	fiber_avl_tree_t *tree;
 *	struct my_data *node;
 *	void *cookie;
 *
 *	cookie = NULL;
 *	while ((node = fiber_avl_destroy_nodes(tree, &cookie)) != NULL)
 *		free(node);
 *	fiber_avl_destroy(tree);
 */
void *fiber_avl_destroy_nodes(fiber_avl_tree_t *tree, void **cookie);


/*
 * Final destroy of an AVL tree. Arguments are:
 *
 * tree   - the empty tree to destroy
 */
void fiber_avl_destroy(fiber_avl_tree_t *tree);

#ifdef	__cplusplus
}
#endif

#endif	/* _AVL_H */
