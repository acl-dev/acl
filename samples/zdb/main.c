#include "lib_acl.h"
#include "zdb_test.h"

int main(int argc acl_unused, char *argv[] acl_unused)
{
	acl_mem_slice_init(8, 1024, 100000,
			ACL_SLICE_FLAG_GC2 |
			ACL_SLICE_FLAG_RTGC_OFF |
			ACL_SLICE_FLAG_LP64_ALIGN);
	if (argc == 2 && strncasecmp(argv[1], "zdb", 3) == 0) {
		acl_msg_open("test.log", "zdb_test");
		zdb_test_main(argv[1]);
	} else
		zdb_test_main("zdb:help");
	return (0);
}
