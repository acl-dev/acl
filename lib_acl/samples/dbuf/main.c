#include "lib_acl.h"

int main(void)
{
	ACL_DBUF_POOL *dbuf = acl_dbuf_pool_create(8192);
	char *huge_ptr;
	int   i;
	
	for (i = 0; i < 10240; i++)
		acl_dbuf_pool_alloc(dbuf, 128);

	huge_ptr = acl_dbuf_pool_alloc(dbuf, 81920);

	for (i = 0; i < 10240; i++)
		acl_dbuf_pool_alloc(dbuf, 128);

	acl_dbuf_pool_free(dbuf, huge_ptr);

	acl_dbuf_pool_destroy(dbuf);

	printf("---------------OK--------------------\r\n");

	return 0;
}
