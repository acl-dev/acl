#ifndef	ACL_STACK_INCLUDE_H
#define	ACL_STACK_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_iterator.h"

/* Note: This function internally uses dynamic memory allocation method */

typedef struct ACL_STACK ACL_STACK;

/**
 * Stack type definition.
 */
struct ACL_STACK {
	int   capacity;
	int   count;
	void **items;

	/* Operation methods */

	/* Append dynamic object to stack tail */
	void  (*push_back)(struct ACL_STACK*, void*);
	/* Prepend dynamic object to stack head */
	void  (*push_front)(struct ACL_STACK*, void*);
	/* Pop dynamic object from stack tail */
	void *(*pop_back)(struct ACL_STACK*);
	/* Pop dynamic object from stack head */
	void *(*pop_front)(struct ACL_STACK*);

	/* for acl_iterator */

	/* Get iterator head pointer */
	void *(*iter_head)(ACL_ITER*, struct ACL_STACK*);
	/* Get next iterator pointer */
	void *(*iter_next)(ACL_ITER*, struct ACL_STACK*);
	/* Get iterator tail pointer */
	void *(*iter_tail)(ACL_ITER*, struct ACL_STACK*);
	/* Get previous iterator pointer */
	void *(*iter_prev)(ACL_ITER*, struct ACL_STACK*);
};

/**
 * Expand stack space size.
 * @param s {ACL_STACK*} Stack object pointer
 * @param count {int} Space size to add
 */
ACL_API void acl_stack_space(ACL_STACK *s, int count);

/**
 * Create a stack object.
 * @param init_size {int} Stack's initial allocated space size, must be > 0
 * @return {ACL_STACK*} Newly created stack object pointer
 */
ACL_API ACL_STACK *acl_stack_create(int init_size);

/**
 * Clear objects in stack, but do not destroy stack object.
 * @param s {ACL_STACK*} Stack object pointer
 * @param free_fn {void (*)(void*)} If not NULL, use this function to free each object in stack
 */
ACL_API void acl_stack_clean(ACL_STACK *s, void (*free_fn)(void *));

/**
 * Destroy stack object's objects and stack object.
 * @param s {ACL_STACK*} Stack object pointer
 * @param free_fn {void (*)(void*)} If not NULL, use this function to free each object in stack
 */
ACL_API void acl_stack_destroy(ACL_STACK *s, void (*free_fn)(void *));

/**
 * Append new object to stack tail.
 * @param s {ACL_STACK*} Stack object pointer
 * @param obj {void*}
 */
ACL_API void acl_stack_append(ACL_STACK *s, void *obj);
#define	acl_stack_push	acl_stack_append

/**
 * Prepend new object to stack head.
 * @param s {ACL_STACK*} Stack object pointer
 * @param obj {void*}
 * Note: This operation's efficiency is lower than acl_stack_append, because
 *  it needs to move all object positions
 */
ACL_API void acl_stack_prepend(ACL_STACK *s, void *obj);

/**
 * Delete a certain object from stack.
 * @param s {ACL_STACK*} Stack object pointer
 * @param position {int} Position in stack array
 * @param free_fn {void (*)(void*)} If not NULL, use this function to free deleted object
 */
ACL_API void acl_stack_delete(ACL_STACK *s, int position, void (*free_fn)(void *));

/**
 * @param s {ACL_STACK*} Stack object pointer
 * @param obj {void*} Address of object to delete
 * @param free_fn {void (*)(void*)} If not NULL, use this function to free deleted object
 */
ACL_API void acl_stack_delete_obj(ACL_STACK *s, void *obj, void (*free_fn)(void *));

/**
 * Get object address at a certain position in stack array.
 * @param s {ACL_STACK*} Stack object pointer
 * @param position {int} Position in stack array
 * @return {void*} != NULL: ok; == NULL: error or does not exist
 */
ACL_API void *acl_stack_index(ACL_STACK *s, int position);

/**
 * Get current number of objects in stack array.
 * @param s {ACL_STACK*} Stack object pointer
 * @return {int} Object count
 */
ACL_API int acl_stack_size(const ACL_STACK *s);

/**
 * Get object address at stack tail, simultaneously remove object from stack.
 * @param s {ACL_STACK*} Stack object pointer
 * @return {void*} Object address, == NULL indicates current stack is empty
 */
ACL_API void *acl_stack_pop(ACL_STACK *s);
#define acl_stack_pop_tail acl_stack_pop

/**
 * Get address of object last added to stack, but do not remove object from stack.
 * @param s {ACL_STACK*} Stack object pointer
 * @return {void*} Object address, == NULL indicates current stack is empty
 */
ACL_API void *acl_stack_top(ACL_STACK *s);

#ifdef	__cplusplus
}
#endif

#endif
