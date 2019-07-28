#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

static char *html = \
"<html>\r\n"
"<head> <title> 'hello world!' </title> </head>\r\n"
"<body> \"hello && world!\" </body>\r\n"
"</html>\r\n";

int main(int argc acl_unused, char *argv[] acl_unused)
{
	ACL_VSTRING *buf1 = acl_vstring_alloc(32);
	ACL_VSTRING *buf2 = acl_vstring_alloc(32);

	acl_xml_encode(html, buf1);
	printf(">>> xml encode: len(%d)\n%s",(int) ACL_VSTRING_LEN(buf1), acl_vstring_str(buf1));

	printf("------------------------------------------\n");

	acl_xml_decode(acl_vstring_str(buf1), buf2);
	printf(">>> xml decode: len(%d)\n%s", (int) ACL_VSTRING_LEN(buf2), acl_vstring_str(buf2));

	printf("------------------------------------------\n");

	ACL_VSTRING_RESET(buf1);
	ACL_VSTRING_RESET(buf2);

	acl_html_encode(html, buf1);
	printf(">>> html encode: len(%d)\n%s", (int) ACL_VSTRING_LEN(buf1), acl_vstring_str(buf1));

	printf("------------------------------------------\n");

	acl_html_decode(acl_vstring_str(buf1), buf2);
	printf(">>> html decode: len(%d)\n%s", (int) ACL_VSTRING_LEN(buf2), acl_vstring_str(buf2));

	acl_vstring_free(buf1);
	acl_vstring_free(buf2);

#ifdef	WIN32
	printf("------------------------------------------\n");
	printf("enter any key to exit ...\n");
	getchar();
#endif
	return (0);
}
