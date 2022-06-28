
#ifndef	_RING_H_INCLUDE_
#define	_RING_H_INCLUDE_

#ifdef  __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct RING RING;

struct RING {
	RING   *succ;			/* successor */
	RING   *pred;			/* predecessor */
};

#define	ring_init(ring_in) do  \
{  \
	RING   *ring = ring_in;  \
	ring->pred = ring->succ = ring;  \
} while (0)

#define	ring_append(ring_in, entry_in) do  \
{  \
	RING   *ring = ring_in;  \
	RING   *entry = entry_in;  \
	entry->succ      = ring->succ;  \
	entry->pred      = ring;  \
	ring->succ->pred = entry;  \
	ring->succ       = entry;  \
} while (0)

#define	ring_prepend(ring_in, entry_in) do  \
{  \
	RING   *ring = ring_in;  \
	RING   *entry = entry_in;  \
	entry->pred      = ring->pred;  \
	entry->succ      = ring;  \
	ring->pred->succ = entry;  \
	ring->pred       = entry;  \
} while (0)

#define	ring_detach(entry_in) do  \
{  \
	RING   *entry = entry_in;  \
	RING   *succ;  \
	RING   *pred;  \
	succ = entry->succ;  \
	pred = entry->pred;  \
	if (succ != NULL && pred != NULL) {  \
		pred->succ = succ;  \
		succ->pred = pred;  \
		entry->succ = entry->pred = 0;  \
	}  \
} while (0)

#define ring_succ(c) ((c)->succ)
#define ring_pred(c) ((c)->pred)

#define RING_TO_APPL(ring_ptr, app_type, ring_member) \
    ((app_type *) (((char *) (ring_ptr)) - offsetof(app_type,ring_member)))

#define	FOREACH_RING_FORWARD(entry, head) \
	for (entry = ring_succ(head); entry != (head); entry = ring_succ(entry))

#define	FOREACH_RING_BACKWARD(entry, head) \
	for (entry = ring_pred(head); entry != (head); entry = ring_pred(entry))

#define FIRST_APPL(head, app_type, ring_member) \
	(ring_succ(head) != (head) ? \
		RING_TO_APPL(ring_succ(head), app_type, ring_member) : 0)

#ifdef  __cplusplus
}
#endif

#endif

