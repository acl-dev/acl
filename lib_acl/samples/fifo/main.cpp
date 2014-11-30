#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

static void fifo_test(bool use_slice)
{
	ACL_FIFO *fifo;
	int   i;
	char *ptr;
	ACL_SLICE_POOL *slice;

	if (use_slice)
		slice = acl_slice_pool_create(10, 100,
			ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF);
	else
		slice = NULL;

	fifo = acl_fifo_new1(slice);

	for (i = 0; i < 20; i++) {
		if (slice)
			ptr = (char*) acl_slice_pool_alloc(__FILE__, __LINE__,
					slice, 100);
		else
			ptr = (char*) acl_mymalloc(100);
		snprintf(ptr, 100, "test:%d", i);
		(void) acl_fifo_push(fifo, ptr);
		printf(">>>ptr: %s\n", ptr);
	}

	while (1) {
		ptr = (char*) acl_fifo_pop(fifo);
		if (ptr == NULL)
			break;
		printf("fifo pop: %s\n", ptr);
		if (!slice)
			acl_myfree(ptr);
	}

	if (slice == NULL)
		acl_fifo_free(fifo, acl_myfree_fn);
	else
		acl_slice_pool_destroy(slice);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	bool use_slice;

	if (argc >= 2 && strcasecmp(argv[1], "slice") == 0)
		use_slice = true;
	else
		use_slice = false;
	fifo_test(use_slice);
	getchar();
	return (0);
}
