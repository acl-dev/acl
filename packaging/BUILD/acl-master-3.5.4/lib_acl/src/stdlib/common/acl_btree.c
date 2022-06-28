/*
 * FILE:     btree.c
 * PROGRAM:  RAT
 * AUTHOR:   O.Hodson
 * MODIFIED: C.Perkins
 * 
 * Binary tree implementation - Mostly verbatim from:
 *
 * Introduction to Algorithms by Corman, Leisserson, and Rivest,
 * MIT Press / McGraw Hill, 1990.
 *
 */

#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_slice.h"
#include "stdlib/acl_btree.h"

#endif

typedef struct BTREE_NODE {
	unsigned int key;
	void *data;
	struct BTREE_NODE *parent;
	struct BTREE_NODE *left;
	struct BTREE_NODE *right;
	unsigned int magic;
} BTREE_NODE;

struct ACL_BTREE {
	BTREE_NODE *root;
	unsigned int magic;
	int count;
	ACL_SLICE *slice;
};

/*****************************************************************************/
/* Debugging functions...                                                    */
/*****************************************************************************/

#define BTREE_MAGIC      0x10101010
#define BTREE_NODE_MAGIC 0x01010101

#ifdef DEBUG
static int btree_count;

static void btree_validate_node(BTREE_NODE *node, BTREE_NODE *parent)
{
	const char *myname = "btree_validate_node";

	if (node->magic != BTREE_NODE_MAGIC)
		acl_msg_fatal("%s(%d): magic(%u) invalid", myname, __LINE__, node->magic);

	if (node->parent != parent)
		acl_msg_fatal("%s(%d): parent invalid", myname, __LINE__);

	btree_count++;

	if (node->left != NULL) {
		btree_validate_node(node->left, node);
	}
	if (node->right != NULL) {
		btree_validate_node(node->right, node);
	}
}

#endif

static void btree_validate(ACL_BTREE *t)
{
	const char *myname = "btree_validate";

	if (t->magic != BTREE_MAGIC)
		acl_msg_fatal("%s(%d): magic(%u) invalid", myname, __LINE__, t->magic);

#ifdef DEBUG
	btree_count = 0;
	if (t->root != NULL) {
		btree_validate_node(t->root, NULL);
	}
	if (btree_count != t->count)
		acl_msg_fatal("%s(%d): btree_count(%d) invalid",
			myname, __LINE__, btree_count);
#endif
}

/*****************************************************************************/
/* Utility functions                                                         */
/*****************************************************************************/

static BTREE_NODE *btree_min(BTREE_NODE *x)
{
	if (x == NULL) {
		return (NULL);
	}
	while(x->left) {
		x = x->left;
	}
	return (x);
}

static BTREE_NODE *btree_max(BTREE_NODE *x)
{
	if (x == NULL) {
		return (NULL);
	}
	while(x->right) {
		x = x->right;
	}
	return (x);
}

static BTREE_NODE *btree_successor(BTREE_NODE *x)
{
	BTREE_NODE *y;

	if (x->right != NULL) {
		return (btree_min(x->right));
	}

	y = x->parent;
	while (y != NULL && x == y->right) {
		x = y;
		y = y->parent;
	}

	return (y);
}

static BTREE_NODE *btree_search(BTREE_NODE *x, unsigned int key)
{
	while (x != NULL && key != x->key) {
		if (key < x->key) {
			x = x->left;
		} else {
			x = x->right;
		}
	}
	return (x); 
}

static void btree_insert_node(ACL_BTREE *tree, BTREE_NODE *z)
{
	const char *myname = "btree_insert_node";
	BTREE_NODE *x, *y;

	btree_validate(tree);
	y = NULL;
	x = tree->root;
	while (x != NULL) {
		y = x;
		if (z->key == x->key)
			acl_msg_fatal("%s(%d): key(%u) exist",
				myname, __LINE__, z->key);
		if (z->key < x->key) {
			x = x->left;
		} else {
			x = x->right;
		}
	}

	z->parent = y;
	if (y == NULL) {
		tree->root = z;
	} else if (z->key < y->key) {
		y->left = z;
	} else {
		y->right = z;
	}
	tree->count++;
	btree_validate(tree);
}

static BTREE_NODE *btree_delete_node(ACL_BTREE *tree, BTREE_NODE *z)
{
	BTREE_NODE *x, *y;

	btree_validate(tree);
	if (z->left == NULL || z->right == NULL) {
		y = z;
	} else {
		y = btree_successor(z);
	}

	if (y->left != NULL) {
		x = y->left;
	} else {
		x = y->right;
	}

	if (x != NULL) {
		x->parent = y->parent;
	}

	if (y->parent == NULL) {
		tree->root = x;
	} else if (y == y->parent->left) {
		y->parent->left = x;
	} else {
		y->parent->right = x;
	}

	z->key  = y->key;
	z->data = y->data;

	tree->count--;

	btree_validate(tree);
	return (y);
}

/*****************************************************************************/
/* Exported functions                                                        */
/*****************************************************************************/

ACL_BTREE *acl_btree_create()
{
	ACL_BTREE *t = (ACL_BTREE*) acl_mymalloc(sizeof(ACL_BTREE));

	if (t) {
		t->count = 0;
		t->magic = BTREE_MAGIC;
		t->root  = NULL;
		t->slice = acl_slice_create("acl_btree", 0,
			sizeof(BTREE_NODE), ACL_SLICE_FLAG_GC1);
		return (t);
	}
	return (NULL);
}

int acl_btree_destroy(ACL_BTREE *tree)
{
	const char *myname = "acl_btree_destroy";

	btree_validate(tree);
	if (tree->root != NULL) {
		acl_msg_error("%s(%d): Tree not empty", myname, __LINE__);
		return (-1);
	}

	acl_slice_destroy(tree->slice);
	acl_myfree(tree);
	return (0);
}

void *acl_btree_find(ACL_BTREE *tree, unsigned int key)
{
	BTREE_NODE *x;

	btree_validate(tree);
	x = btree_search(tree->root, key);
	if (x != NULL) {
		return (x->data);
	}
	return (NULL);
}

int acl_btree_add(ACL_BTREE *tree, unsigned int key, void *data)
{
	const char *myname = "acl_btree_add";
	BTREE_NODE *x;

	btree_validate(tree);
	x = btree_search(tree->root, key);
	if (x != NULL) {
		acl_msg_error("%s(%d): Item already exists - key %u",
			myname, __LINE__, key);
		return (-1);
	}

	x = (BTREE_NODE *) acl_slice_alloc(tree->slice);
	x->key = key;
	x->data   = data;
	x->parent = NULL;
	x->left   = NULL;
	x->right  = NULL;
	x->magic  = BTREE_NODE_MAGIC;
	btree_insert_node(tree, x);

	return (0);
}

void *acl_btree_remove(ACL_BTREE *tree, unsigned int key)
{
	const char *myname = "acl_btree_remove";
	BTREE_NODE *x;
	void *data;

	btree_validate(tree);
	x = btree_search(tree->root, key);
	if (x == NULL) {
		acl_msg_error("%s(%d): Item not on tree - key %u\n",
			myname, __LINE__, key);
		return (NULL);
	}

	/* Note value that gets freed is not necessarily the the same
	 * as node that gets removed from tree since there is an
	 * optimization to avoid pointer updates in tree which means
	 * sometimes we just copy key and data from one node to another.  
	 */

	data = x->data;
	x = btree_delete_node(tree, x);
	acl_slice_free2(tree->slice, x);

	return (data);
}

int acl_btree_get_min_key(ACL_BTREE *tree, unsigned int *key)
{
	BTREE_NODE *x;

	btree_validate(tree);
	if (tree->root == NULL) {
		return (-1);
	}

	x = btree_min(tree->root);
	if (x == NULL) {
		return (-1);
	}
        
	*key = x->key;
	return (0);
}

int acl_btree_get_max_key(ACL_BTREE *tree, unsigned int *key)
{
	BTREE_NODE *x;

	btree_validate(tree);
	if (tree->root == NULL) {
		return (-1);
	}

	x = btree_max(tree->root);
	if (x == NULL) {
		return (-1);
	}
        
	*key = x->key;
	return (0);
}

int acl_btree_get_next_key(ACL_BTREE *tree,
	unsigned int cur_key, unsigned int *next_key)
{
	BTREE_NODE *x;

	btree_validate(tree);
	x = btree_search(tree->root, cur_key);
	if (x == NULL) {
		return (-1);
	}
        
	x = btree_successor(x);
	if (x == NULL) {
		return (-1);
	}
        
	*next_key = x->key;
	return (0);
}

static int btree_depth(BTREE_NODE *x)
{
	int l, r;

	if (x == NULL) {
		return (0);
	}

	l = btree_depth(x->left);
	r = btree_depth(x->right);

	if (l > r) {
		return (l + 1);
	} else {
		return (r + 1);
	}
}

int acl_btree_depth(ACL_BTREE *tree)
{
	return (btree_depth(tree->root));
}

static void btree_dump_node(BTREE_NODE *x, int depth, int c, int w)
{
	int   i;

	if (x == NULL) {
		return;
	}

	/* move(depth * 2, c); */
	for (i = 0; i < depth * 2; i++)	{
		printf("\r\n");
	}
	for (i = 0; i < c; i++)	{
		printf("%c", ' ');
	}
	fflush(stdout);

	printf("%u\r\n", x->key);

	btree_dump_node(x->left,  depth + 1, c - w/2, w/2);
	btree_dump_node(x->right, depth + 1, c + w/2, w/2);
}

void acl_btree_dump(ACL_BTREE *b)
{
	btree_dump_node(b->root, 0, 40, 48);
}
