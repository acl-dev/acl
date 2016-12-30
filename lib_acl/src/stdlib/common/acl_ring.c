/* Application-specific. */

#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_ring.h"

#endif

/* acl_ring_init - initialize ring head */
void acl_ring_init(ACL_RING *ring)
{
	if (ring == NULL)
		return;
	ring->pred   = ring->succ = ring;
	ring->parent = ring;
	ring->len    = 0;
}

/* acl_ring_size - the entry number in the ring */

int acl_ring_size(const ACL_RING *ring)
{
	if (ring == NULL)
		return -1;

	return ring->len;
}

/* acl_ring_append - insert entry after ring head */

void acl_ring_append(ACL_RING *ring, ACL_RING *entry)
{
	if (ring == NULL || entry == NULL)
		return;
	entry->succ      = ring->succ;
	entry->pred      = ring;
	entry->parent    = ring->parent;
	ring->succ->pred = entry;
	ring->succ       = entry;
	ring->parent->len++;
}

/* acl_ring_prepend - insert new entry before ring head */

void acl_ring_prepend(ACL_RING *ring, ACL_RING *entry)
{
	if (ring == NULL || entry == NULL)
		return;
	entry->pred      = ring->pred;
	entry->succ      = ring;
	entry->parent    = ring->parent;
	ring->pred->succ = entry;
	ring->pred       = entry;
	ring->parent->len++;
}

/* acl_ring_detach - remove entry from ring */

void acl_ring_detach(ACL_RING *entry)
{
	ACL_RING   *succ;
	ACL_RING   *pred;

	if (entry == NULL || entry->parent == entry)
		return;
	succ = entry->succ;
	pred = entry->pred;
	if (succ == NULL || pred == NULL)
		return;
	pred->succ = succ;
	succ->pred = pred;

	entry->parent->len--;

	entry->succ = entry->pred = entry;
	entry->parent = entry;
	entry->len = 0;
}

/* acl_ring_pop_head - pop ring's head entry out from ring */

ACL_RING *acl_ring_pop_head(ACL_RING *ring)
{
	ACL_RING   *succ;

	if (ring == NULL)
		return NULL;

	succ = ring->succ;
	if (succ == ring)
		return NULL;

	acl_ring_detach(succ);

	return succ;
}

/* acl_ring_pop_tail - pop ring's tail entry out from ring */

ACL_RING *acl_ring_pop_tail(ACL_RING *ring)
{
	ACL_RING   *pred;

	if (ring == NULL)
		return NULL;

	pred = ring->pred;
	if (pred == ring)
		return NULL;

	acl_ring_detach(pred);

	return pred;
}
