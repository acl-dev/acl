#include "lib_acl.h"

int main(void)
{
	ACL_VSTRING *buf = acl_vstring_alloc(100);
	const char *s = "hello world\r\nhello world1\nhello world2\r\nhello world3";
	const char *p = s, *p1;
	ssize_t n = (ssize_t) strlen(p);

	while (n > 0) {
		ACL_VSTRING_RESET(buf);
		p1 = p;
		if (acl_buffer_gets_nonl(buf, &p, (size_t) n) == NULL
			&& ACL_VSTRING_LEN(buf) == 0)
		{
			break;
		}
		printf(">>%s\n", acl_vstring_str(buf));
		n -=  p - p1;
	}

	acl_vstring_free(buf);
	return (0);
}
