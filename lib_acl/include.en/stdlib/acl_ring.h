#ifndef	ACL_RING_INCLUDE_H
#define	ACL_RING_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <stddef.h>

typedef struct ACL_RING ACL_RING;

/**
 * Ring buffer structure type definition.
 */
struct ACL_RING {
	ACL_RING *succ;           /**< successor */
	ACL_RING *pred;           /**< predecessor */

	ACL_RING *parent;         /**< the header of all the rings */
	int len;                  /**< the count in the ring */
};

typedef struct ACL_RING_ITER {
	ACL_RING *ptr;
} ACL_RING_ITER;

/**
 * Initialize ring buffer.
 * @param ring {ACL_RING*} Ring buffer
 */
ACL_API void acl_ring_init(ACL_RING *ring);

/**
 * Get current ring buffer's element count.
 * @param ring {ACL_RING*} Ring buffer
 * @return {int} Ring buffer's element count
 */
ACL_API int  acl_ring_size(const ACL_RING *ring);

/**
 * Insert a new element before ring tail. In ring, position after inserted
 * element is after inserted element.
 * @param ring {ACL_RING*} Ring buffer
 * @param entry {ACL_RING*} New element
 */
ACL_API void acl_ring_prepend(ACL_RING *ring, ACL_RING *entry);

/**
 * Insert a new element before ring head. In ring, position after inserted
 * element is before inserted element.
 * @param ring {ACL_RING*} Ring buffer
 * @param entry {ACL_RING*} New element
 */
ACL_API void acl_ring_append(ACL_RING *ring, ACL_RING *entry);

/**
 * Remove a certain element from ring buffer.
 * @param entry {ACL_RING*} Element
 */
ACL_API void acl_ring_detach(ACL_RING *entry);

/**
 * Pop head element from ring.
 * @param ring {ACL_RING*} Ring buffer
 * @return {ACL_RING*} Head element, if NULL, indicates ring buffer is empty
 */
ACL_API ACL_RING *acl_ring_pop_head(ACL_RING *ring);

/**
 * Pop tail element from ring.
 * @param ring {ACL_RING*} Ring buffer
 * @return {ACL_RING*} Tail element, if NULL, indicates ring buffer is empty
 */
ACL_API ACL_RING *acl_ring_pop_tail(ACL_RING *ring);

/*--------------------  Some helper macros --------------------------------*/

/**
 * Get current element's next element.
 */
#define ACL_RING_SUCC(c) ((c)->succ)
#define	acl_ring_succ	ACL_RING_SUCC

/**
 * Get current element's previous element.
 */
#define ACL_RING_PRED(c) ((c)->pred)
#define	acl_ring_pred	ACL_RING_PRED

/**
 * Convert element pointer to application's custom object type pointer address.
 * @param ring_ptr {ACL_RING*} Element pointer
 * @param app_type Application's custom type
 * @param ring_member {ACL_RING*} Element member name in application's custom
 *  object structure
 * @return {app_type*} Application's custom object structure type's object address
 */
#define ACL_RING_TO_APPL(ring_ptr, app_type, ring_member) \
    ((app_type *) (((char *) (ring_ptr)) - offsetof(app_type,ring_member)))

#define	acl_ring_to_appl	ACL_RING_TO_APPL

/**
 * Iterate from head to tail through all ring elements in ring buffer.
 * @param iter {ACL_RING_ITER}
 * @param head_ptr {ACL_RING*} Ring buffer's head pointer
 * @example:
 	typedef struct {
		char  name[32];
		ACL_RING entry;
	} DUMMY;

	void test()
	{
		ACL_RING head;
		DUMMY *dummy;
		ACL_RING_ITER iter;
		int   i;

		acl_ring_init(&head);

		for (i = 0; i < 10; i++) {
			dummy = (DUMMY*) acl_mycalloc(1, sizeof(DUMMY));
			snprintf(dummy->name, sizeof(dummy->name), "dummy:%d", i);
			acl_ring_append(&head, &dummy->entry);
		}

		acl_ring_foreach(iter, &head) {
			dummy = acl_ring_to_appl(iter.ptr, DUMMY, entry);
			printf("name: %s\n", dummy->name);
		}

		while (1) {
			iter.ptr = acl_ring_pop_head(&head);
			if (iter.ptr == NULL)
				break;
			dummy = acl_ring_to_appl(iter.ptr, DUMMY, entry);
			acl_myfree(dummy);
		}
	}
 */
#define	ACL_RING_FOREACH(iter, head_ptr) \
        for ((iter).ptr = acl_ring_succ((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = acl_ring_succ((iter).ptr))

#define	acl_ring_foreach		ACL_RING_FOREACH

/**
 * Iterate from tail to head through all ring elements in ring buffer.
 * @param iter {ACL_RING_ITER}
 * @param head_ptr {ACL_RING*} Ring buffer's head pointer
 */
#define	ACL_RING_FOREACH_REVERSE(iter, head_ptr) \
        for ((iter).ptr = acl_ring_pred((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = acl_ring_pred((iter).ptr))

#define	acl_ring_foreach_reverse	ACL_RING_FOREACH_REVERSE

/**
 * Get first element pointer in ring buffer.
 * @param head {ACL_RING*} Head pointer
 * @return {ACL_RING*} NULL: ring is empty
 */
#define ACL_RING_FIRST(head) \
	(acl_ring_succ(head) != (head) ? acl_ring_succ(head) : 0)

#define	acl_ring_first		ACL_RING_FIRST

/**
 * Get first element pointer in ring buffer and simultaneously convert to
 * application's custom object structure type's object address.
 * @param head {ACL_RING*} Head pointer
 * @param app_type Application's custom object structure type
 * @param ring_member {ACL_RING*} Element member name in application's custom object structure
 * @return {app_type*} Application's custom object structure type's object address
 */
#define ACL_RING_FIRST_APPL(head, app_type, ring_member) \
	(acl_ring_succ(head) != (head) ? \
	 ACL_RING_TO_APPL(acl_ring_succ(head), app_type, ring_member) : 0)

#define	acl_ring_first_appl	ACL_RING_FIRST_APPL

/**
 * Get last element pointer in ring buffer.
 * @param head {ACL_RING*} Head pointer
 * @return {ACL_RING*} NULL: ring is empty
 */
#define ACL_RING_LAST(head) \
       (acl_ring_pred(head) != (head) ? acl_ring_pred(head) : 0)

#define	acl_ring_last		ACL_RING_LAST

/**
 * Get last element pointer in ring buffer and simultaneously convert to
 * application's custom object structure type's object address.
 * @param head {ACL_RING*} Head pointer
 * @param app_type Application's custom object structure type
 * @param ring_member {ACL_RING*} Element member name in application's custom object structure
 * @return {app_type*} Application's custom object structure type's object address
 */
#define ACL_RING_LAST_APPL(head, app_type, ring_member) \
       (acl_ring_pred(head) != (head) ? \
	ACL_RING_TO_APPL(acl_ring_pred(head), app_type, ring_member) : 0)

#define	acl_ring_last_appl	ACL_RING_LAST_APPL

/**
 * Insert a new element before ring tail.
 * @param ring {ACL_RING*} Ring buffer
 * @param entry {ACL_RING*} New element
 */
#define	ACL_RING_APPEND(ring_in, entry_in) do {  \
	ACL_RING *ring_ptr = (ring_in), *entry_ptr = (entry_in);  \
        entry_ptr->succ      = ring_ptr->succ;  \
        entry_ptr->pred      = ring_ptr;  \
        entry_ptr->parent    = ring_ptr->parent;  \
        ring_ptr->succ->pred = entry_ptr;  \
        ring_ptr->succ       = entry_ptr;  \
        ring_ptr->parent->len++;  \
} while (0)

/**
 * Insert a new element before ring head.
 * @param ring {ACL_RING*} Ring buffer
 * @param entry {ACL_RING*} New element
 */
#define	ACL_RING_PREPEND(ring_in, entry_in) do {  \
	ACL_RING *ring_ptr = (ring_in), *entry_ptr = (entry_in);  \
	entry_ptr->pred      = ring_ptr->pred;  \
	entry_ptr->succ      = ring_ptr;  \
	entry_ptr->parent    = ring_ptr->parent;  \
	ring_ptr->pred->succ = entry_ptr;  \
	ring_ptr->pred       = entry_ptr;  \
	ring_ptr->parent->len++;  \
} while (0)

/**
 * Remove a certain element from ring buffer.
 * @param entry {ACL_RING*} Element
 */
#define	ACL_RING_DETACH(entry_in) do {  \
	ACL_RING   *succ, *pred, *entry_ptr = (entry_in);  \
	succ = entry_ptr->succ;  \
	pred = entry_ptr->pred;  \
	if (succ != NULL && pred != NULL) {  \
		pred->succ = succ;  \
		succ->pred = pred;  \
		entry_ptr->parent->len--;  \
		entry_ptr->succ = entry_ptr->pred = NULL;  \
	}  \
} while (0)

#ifdef  __cplusplus
}
#endif

#endif
