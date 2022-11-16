#include "stdafx.h"
#include "common.h"

#include "fiber.h"
#include "sync_type.h"

SYNC_OBJ *sync_obj_alloc(int shared)
{
	SYNC_OBJ *obj = (SYNC_OBJ*) mem_calloc(1, sizeof(SYNC_OBJ));

	ring_init(&obj->me);
	if (shared) {
		obj->atomic = atomic_new();
		atomic_set(obj->atomic, &obj->atomic_value);
		atomic_int64_set(obj->atomic, 1);
	}
	return obj;
}

static void sync_obj_free(SYNC_OBJ *obj)
{
	if (obj->atomic) {
		atomic_free(obj->atomic);
	}
	if (obj->base) {
		fbase_event_close(obj->base);
		fbase_free(obj->base);
	}
	mem_free(obj);
}

unsigned sync_obj_refer(SYNC_OBJ *obj)
{
	if (obj->atomic) {
		return (unsigned) atomic_int64_add_fetch(obj->atomic, 1);
	}
	return 1;
}

unsigned sync_obj_unrefer(SYNC_OBJ *obj)
{
	if (obj->atomic) {
		long long n;

		if ((n = atomic_int64_add_fetch(obj->atomic, -1)) == 0) {
			sync_obj_free(obj);
			return 0;
		}
		return (unsigned) n;
	}

	sync_obj_free(obj);
	return 0;
}
