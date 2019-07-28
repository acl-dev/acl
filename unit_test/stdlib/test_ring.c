
#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#include "test_stdlib.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

typedef struct MY_TYPE {
	char  name[256];
	char  value[256];
	int   i;

	ACL_RING ring_entry;
} MY_TYPE;

static ACL_RING __ring_header;

static void __create_ring(int flag, int size)
{
	MY_TYPE *my_type;
	int   i;

	for (i = 0; i < size; i++) {
		my_type = acl_mycalloc(1, sizeof(MY_TYPE));
		snprintf(my_type->name, sizeof(my_type->name), "name:%d", i);
		snprintf(my_type->value, sizeof(my_type->value), "value:%d", i);
		my_type->i = i;
		if (flag)
			acl_ring_prepend(&__ring_header, &my_type->ring_entry);
		else
			acl_ring_append(&__ring_header, &my_type->ring_entry);
	}
}

int test_ring(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	ACL_RING_ITER ring_iter;
	MY_TYPE *my_type;

	acl_ring_init(&__ring_header);

	__create_ring(0, 10);

	printf("\nring loop printf:> len=%d\n", acl_ring_size(&__ring_header));

	acl_ring_foreach(ring_iter, &__ring_header) {
		my_type = ACL_RING_TO_APPL(ring_iter.ptr, MY_TYPE, ring_entry);
		printf("name=%s, value=%s\n", my_type->name, my_type->value);
	}

	printf("\nring pop head loop printf:> len=%d\n", acl_ring_size(&__ring_header));

	while (1) {
		ring_iter.ptr = acl_ring_pop_head(&__ring_header);
		if (ring_iter.ptr == NULL)
			break;

		my_type = ACL_RING_TO_APPL(ring_iter.ptr, MY_TYPE, ring_entry);
		printf("name=%s, value=%s\n", my_type->name, my_type->value);
		acl_myfree(my_type);
	}

	printf("first end, len=%d\n", acl_ring_size(&__ring_header));

	__create_ring(1, 20);

	printf("\nring loop printf:> len=%d\n", acl_ring_size(&__ring_header));

	acl_ring_foreach_reverse(ring_iter, &__ring_header) {
		my_type = ACL_RING_TO_APPL(ring_iter.ptr, MY_TYPE, ring_entry);
		printf("name=%s, value=%s\n", my_type->name, my_type->value);
	}

	printf("\nring pop tail loop printf:> len=%d\n", acl_ring_size(&__ring_header));

	while (1) {
		ring_iter.ptr = acl_ring_pop_tail(&__ring_header);
		if (ring_iter.ptr == NULL)
			break;

		my_type = ACL_RING_TO_APPL(ring_iter.ptr, MY_TYPE, ring_entry);
		printf("name=%s, value=%s\n", my_type->name, my_type->value);
		acl_myfree(my_type);
	}

	return (0);
}

