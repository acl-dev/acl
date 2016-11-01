
#if defined(ACL_WINDOWS)
#pragma comment(lib,"ws2_32")
#endif

#include "lib_acl.h"
#include <assert.h>

static void test_urlcode(void)
{
	const char *params = "n a m e=中国&value=人民&name2=姓名&value2=逍遥仙";
	char *tmp1, *tmp2;

	printf("params: (%s), len=%d\r\n", params, (int) strlen(params));
	tmp1 = acl_url_encode(params, NULL);
	assert(tmp1);
	printf("encode: (%s), len=%d\r\n", tmp1, (int) strlen(tmp1));
	tmp2 = acl_url_decode(tmp1, NULL);
	assert(tmp2);
	printf("decode: (%s), len=%d\r\n", tmp2, (int) strlen(tmp2));

	acl_myfree(tmp1);
	acl_myfree(tmp2);
}

int main(void)
{
	test_urlcode();
	printf(">>\\r: %d, \\n: %d\r\n", (int) '\r', (int) '\n');

	const char* s = "hello&#13;&#10;world!&#13;&#10;";
	ACL_VSTRING* buf = acl_vstring_alloc(100),
		* buf2 = acl_vstring_alloc(100);
	acl_html_decode(s, buf);
	printf("{%s}\r\n", acl_vstring_str(buf));

	ACL_VSTRING_RESET(buf);
	acl_xml_decode(s, buf);
	printf("{%s}\r\n", acl_vstring_str(buf));

	acl_html_encode(acl_vstring_str(buf), buf2);
	printf("encode: %s\r\n", acl_vstring_str(buf2));

	acl_vstring_free(buf);
	acl_vstring_free(buf2);

	getchar();
	return (0);
}
