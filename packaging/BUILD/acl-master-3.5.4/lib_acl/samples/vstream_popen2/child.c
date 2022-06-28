#include "lib_acl.h"

int main(int argc acl_unused, char *argv[] acl_unused)
{
	char  buf[1024];
	int   ret, i = 0;

	ACL_SAFE_STRNCPY(buf, "first line from child", sizeof(buf));
	while (1) {
		ret = acl_vstream_fprintf(ACL_VSTREAM_OUT,
			">>>send to parent from child(%d)(%s)\n", i, buf);
		if (ret == ACL_VSTREAM_EOF) {
			printf("(child): write error(%s)\n", acl_last_serror());
			break;
		}
		ret = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF) {
			printf("(child): read over(%s)\n", acl_last_serror());
			break;
		}
		if (i++ > 10)
			break;
	}

	return (0);
}
