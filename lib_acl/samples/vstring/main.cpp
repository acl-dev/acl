#include "lib_acl.h"

static void end(void)
{
#ifdef ACL_MS_WINDOWS
	getchar();
#endif
}

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN

static void string_find(ACL_VSTRING *vp, const char *needle)
{
	char *ptr;

	ptr = acl_vstring_strstr(vp, needle);
	if (ptr)
		printf(">>>acl_vstring_strstr: find %s from %s, ptr: %s\r\n", needle, STR(vp), ptr);
	else
		printf(">>>acl_vstring_strstr: not find %s from %s\r\n", needle, STR(vp));
}

static void string_case_find(ACL_VSTRING *vp, const char *needle)
{
	char *ptr;

	ptr = acl_vstring_strcasestr(vp, needle);
	if (ptr)
		printf(">>>acl_vstring_strcasestr: find %s from %s, ptr: %s\r\n", needle, STR(vp), ptr);
	else
		printf(">>>acl_vstring_strcasestr: not find %s from %s\r\n", needle, STR(vp));
}

static void string_rfind(ACL_VSTRING *vp, const char *needle)
{
	char *ptr;

	ptr = acl_vstring_rstrstr(vp, needle);
	if (ptr)
		printf(">>>acl_vstring_rstrstr: find %s from %s, ptr: %s\r\n", needle, STR(vp), ptr);
	else
		printf(">>>acl_vstring_rstrstr: not find %s from %s\r\n", needle, STR(vp));
}

static void string_case_rfind(ACL_VSTRING *vp, const char *needle)
{
	char *ptr;

	ptr = acl_vstring_rstrcasestr(vp, needle);
	if (ptr)
		printf(">>>acl_vstring_rstrcasestr: find %s from %s, ptr: %s\r\n", needle, STR(vp), ptr);
	else
		printf(">>>acl_vstring_rstrcasestr: not find %s from %s\r\n", needle, STR(vp));
}

static const char *needle_tab[] = {
	"h",
	"el",
	"o",
	"e",
	"l",
	"lo",
	"he",
	"He",
	"hello",
	"hel",
	"Hel",
	"helo",
	NULL
};

static const char *string_tab[] = {
	"test test\r\n",
	"test test\n",
	"test",
	"test test",
	"\r\n",
	"test",
	"test test",
	"\n",
	"test\r\ntesttest\r\ntest test",
	"test test",
	"test test",
	"\r\n",
};

static void test_buffer_gets(void)
{
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	const ACL_VSTRING *pbuf;
	const char *src;
	size_t   i, n;
	unsigned char ch = (unsigned char) -1;

	for (i = 0; i < sizeof(string_tab) / sizeof(char*); i++) {
		printf("%s", string_tab[i]);
	}

	printf("------------------------------------------------------------\r\n");

	for (i = 0; i < sizeof(string_tab) / sizeof(char*); i++) {
		src = string_tab[i];
		n = strlen(src);
		while (1) {
			const char *psrc_saved = src;
			pbuf = acl_buffer_gets_nonl(buf, &src, n);
			if (pbuf) {
				printf("%s\n", STR(pbuf));
				ACL_VSTRING_RESET(buf);
				n -= src - psrc_saved;
				if (n == 0)
					break;
			} else
				break;
		}
	}

	printf("------------------------------------------------------------\r\n");

	for (i = 0; i < sizeof(string_tab) / sizeof(char*); i++) {
		src = string_tab[i];
		n = strlen(src);
		while (1) {
			const char *psrc_saved = src;
			pbuf = acl_buffer_gets(buf, &src, n);
			if (pbuf) {
				printf("%s", STR(pbuf));
				ACL_VSTRING_RESET(buf);
				n -= src - psrc_saved;
				if (n == 0)
					break;
			} else
				break;
		}
	}

	acl_vstring_free(buf);
	printf(">>>>>>>>>max ch: %d\n", ch);
}

static void test_buffer_space()
{
	ACL_VSTRING *vp = acl_vstring_alloc(1);
	char *ptr;
	int   i;

	ACL_VSTRING_SPACE(vp, 10);
	ptr = acl_vstring_str(vp);
	printf("=========================1====================\n");
	for (i = 0; i < 10; i++)
		*ptr++ = 'x';
	ptr = acl_vstring_str(vp);
	printf("gets: %d\n", acl_vstream_gets(ACL_VSTREAM_IN, ptr, 10));
	printf("=========================2====================\n");
	acl_vstring_free(vp);
}

static void test_vstring_vsprintf(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	ACL_VSTRING *vbf = acl_vstring_alloc(1);
	acl_vstring_vsprintf(vbf, fmt, ap);
	va_end(ap);

	printf("%s\n", acl_vstring_str(vbf));

	acl_vstring_free(vbf);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	ACL_VSTRING *vp = acl_vstring_alloc(256);
	char *ptr;
	int   i;
	size_t z;

	acl_vstring_strcpy(vp, "大家好，中国人民，Hi大家好, Hello World! 中国人民银行!");

	printf(">>>%s\r\n", acl_vstring_str(vp));
	ptr = acl_vstring_strstr(vp, "Hello");
	if (ptr)
		printf(">>ok, find it, ptr = %s\r\n", ptr);
	else
		printf(">>error, no find it\r\n");

	ptr = acl_vstring_strcasestr(vp, "WORLD");
	if (ptr)
		printf(">>ok, find it, ptr = %s\r\n", ptr);
	else
		printf(">>error, no find it\r\n");

	ptr = acl_vstring_strstr(vp, "中国");
	if (ptr)
		printf(">>ok, find it, ptr = %s\r\n", ptr);
	else
		printf(">>error, no find it\r\n");

	ptr = acl_vstring_strcasestr(vp, "Hi大家好");
	if (ptr)
		printf(">>ok, find it, ptr = %s\r\n", ptr);
	else
		printf(">>error, no find it\r\n");

	ptr = acl_vstring_memchr(vp, 'W');
	if (ptr)
		printf(">>ok, find it, ptr = %s\r\n", ptr);
	else
		printf(">>error, no find it\r\n");

	acl_vstring_strcpy(vp, "hello");
	ptr = acl_vstring_strstr(vp, "h");
	if (ptr)
		printf(">>>find it, ptr: %s\r\n", ptr);
	else
		printf(">>>not find it\r\n");

	printf("\r\n------------------------------------------------------------\r\n");
	for (i = 0; needle_tab[i]; i++) {
		string_rfind(vp, needle_tab[i]);
	}

	printf("\r\n------------------------------------------------------------\r\n");
	for (i = 0; needle_tab[i]; i++) {
		string_case_rfind(vp, needle_tab[i]);
	}

	printf("\r\n------------------------------------------------------------\r\n");
	for (i = 0; needle_tab[i]; i++) {
		string_find(vp, needle_tab[i]);
	}

	printf("\r\n------------------------------------------------------------\r\n");
	for (i = 0; needle_tab[i]; i++) {
		string_case_find(vp, needle_tab[i]);
	}

	printf("\r\n------------------------------------------------------------\r\n");
	const char *s1 = "hello world", *s2 = "WOrld", *s3 = "world";

	printf("strrncasecmp: %s %s %s, n: %d\n", s1,
		strrncasecmp(s1, s2, strlen(s2)) == 0 ? "==" : "!=", s2, (int) strlen(s2));
	printf("strrncasecmp: %s %s %s, n: %d\n", s1,
		strrncasecmp(s1, s2, strlen(s2) + 1) == 0 ? "==" : "!=", s2, (int) strlen(s2) + 1);

	s1 = "www.hexun.com";
	s2 = ".hexun.com";
	printf("strrncasecmp: %s %s %s, n: %d\n", s1,
		strrncasecmp(s1, s2, strlen(s2)) == 0 ? "==" : "!=", s2, (int) strlen(s2));

	printf("\r\n------------------------------------------------------------\r\n");
	printf("strrncmp: %s %s %s, n: %d\n", s1, strrncmp(s1, s2, strlen(s2)) == 0 ? "==" : "!=", s2, (int) strlen(s2));
	printf("strrncmp: %s %s %s, n: 3\n", s1, strrncmp(s1, s2, 3) == 0 ? "==" : "!=", s2);
	printf("strrncmp: %s %s %s, n: %d\n", s1, strrncmp(s1, s3, strlen(s3)) == 0 ? "==" : "!=", s3, (int) strlen(s3));
	printf("strrncmp: %s %s %s, n: %d\n", s1, strrncmp(s1, s3, strlen(s3) + 1) == 0 ? "==" : "!=",
		s3, (int) strlen(s3) + 1);

	z = 1000;
	acl_vstring_sprintf(vp, "max long long int: %llu, size_t: %d", (acl_uint64) -1, (int) z);
	printf("%s\n", acl_vstring_str(vp));
	acl_vstring_free(vp);

	printf("\r\n------------------------------------------------------------\r\n");
	test_buffer_gets();
	test_buffer_space();

	test_vstring_vsprintf("test: %s", s1);

	ACL_VSTRING* vv = acl_vstring_alloc(1);
	acl_vstring_memcpy(vv, "hello", strlen("hello"));
	printf(">>>after acl_vstring_memcpy: [%s], len: %d\r\n",
		acl_vstring_str(vv), (int) ACL_VSTRING_LEN(vv));
	ACL_VSTRING_RESET(vv);
	printf(">>>after ACL_VSTRING_RESET: [%s], len: %d\r\n",
		acl_vstring_str(vv), (int) ACL_VSTRING_LEN(vv));
	ACL_VSTRING_TERMINATE(vv);
	printf(">>>after ACL_VSTRING_TERMINATE: [%s], len: %d\r\n",
		acl_vstring_str(vv), (int) ACL_VSTRING_LEN(vv));
	acl_vstring_free(vv);
	end();
	return (0);
}
