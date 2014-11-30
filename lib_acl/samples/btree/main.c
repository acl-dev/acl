#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
	ACL_BTREE *b;
	unsigned int i, *x;
	unsigned int v[] = {15, 5, 16, 3, 12, 20, 10, 13, 18, 23, 6, 7}; 
	unsigned int nv = sizeof(v) / sizeof(v[0]);

	b = acl_btree_create();

	for(i = 0; i < nv; i++) {
		x = (unsigned int*) acl_mymalloc(sizeof(unsigned int));
		*x = (unsigned int) i;
		if (acl_btree_add(b, v[i], (void*) x) != 0) {
			printf("Fail Add %u %u\r\n", v[i], *x);
		}
	}

	printf("depth %d\n", acl_btree_depth(b));
	acl_btree_dump(b);

	sleep(3);
	x = acl_btree_remove(b, 5);
	if (x)
		acl_myfree(x);
	acl_btree_dump(b);
	sleep(3);
	x = acl_btree_remove(b, 16);
	if (x)
		acl_myfree(x);
	acl_btree_dump(b);
	sleep(3);
	x = acl_btree_remove(b, 13);
	if (x)
		acl_myfree(x);
	acl_btree_dump(b);

	while (acl_btree_get_min_key(b, &i) == 0) {
		if ((x = acl_btree_remove(b, i)) == NULL) {
			fprintf(stderr, "Failed to remove %u\r\n", i);
		} else
			acl_myfree(x);
		acl_btree_dump(b);
		sleep(1); 
	}

	if (acl_btree_destroy(b) == -1) {
		printf("Failed to destroy \r\n");
	}

	return (0);
}

