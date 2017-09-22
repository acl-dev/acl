#include "stdafx.h"

#include "ring.h"

/* ring_init - initialize ring head */
void ring_init(RING *ring)
{
	if (ring == NULL)
		return;
	ring->pred   = ring->succ = ring;
	ring->parent = ring;
	ring->len    = 0;
}

/* ring_size - the entry number in the ring */

int ring_size(const RING *ring)
{
	if (ring == NULL)
		return -1;

	return ring->len;
}

/* ring_append - insert entry after ring head */

void ring_append(RING *ring, RING *entry)
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

/* ring_prepend - insert new entry before ring head */

void ring_prepend(RING *ring, RING *entry)
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

/* ring_detach - remove entry from ring */

void ring_detach(RING *entry)
{
	RING   *succ;
	RING   *pred;

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

/* ring_pop_head - pop ring's head entry out from ring */

RING *ring_pop_head(RING *ring)
{
	RING   *succ;

	if (ring == NULL)
		return NULL;

	succ = ring->succ;
	if (succ == ring)
		return NULL;

	ring_detach(succ);

	return succ;
}

/* ring_pop_tail - pop ring's tail entry out from ring */

RING *ring_pop_tail(RING *ring)
{
	RING   *pred;

	if (ring == NULL)
		return NULL;

	pred = ring->pred;
	if (pred == ring)
		return NULL;

	ring_detach(pred);

	return pred;
}
