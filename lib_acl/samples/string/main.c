#include "lib_acl.h"

static void quote_split(const char* str, const char* delim)
{
	ACL_ITER iter;
	ACL_ARGV *tokens = acl_argv_quote_split(str, delim);

	printf("---------------------------------------------\r\n");
	printf("str = [%s], delim = [%s]\r\n", str, delim);
	acl_foreach(iter, tokens) {
		const char* ptr = (const char*) iter.data;
		printf("%s\r\n", ptr);
	}

	acl_argv_free(tokens);
}

static void test_quote_split(void)
{
	struct {
		const char* str;
		const char* delim;
	} data[] = {
		{ "hello world", " \t" },
		{ "\t hello\' world\"", "\t " },
		{ "hi \"hello world\"", "\t " },
		{ "\t   'myname is zsx.'    \"what\'s your name?\"   Good! ", "\t " },
		{ "name = \"zsx xsz\", name2 = 'xsz,= zsx'", "\t =," },
		{ NULL, NULL },
	};
	int i;

	for (i = 0; data[i].str != NULL && data[i].delim; i++)
		quote_split(data[i].str, data[i].delim);
}

int main(void)
{
	char *src = acl_mystrdup("hello \tworld! you're  welcome to China!");
	char *ptr, *src_saved;
	ACL_VSTRING* buf;
	const char* s = "hello";
	unsigned int   n1 = (unsigned int) -1;
	unsigned long  n2 = (unsigned long) -1;
	unsigned long long n3 = (unsigned long long) -1;
	const char *str2 = "hello world, you're welcome!";
	ACL_ARGV *tokens = acl_argv_split(str2, " \t,'!");
	ACL_ITER  iter;

	printf("----------------------------------------------\r\n");
	acl_foreach(iter, tokens)
		printf("tokens[%d]: %s\r\n", iter.i, (const char*) iter.data);
	printf("total: %d\r\n", iter.size);
	acl_argv_free(tokens);
	printf("----------------------------------------------\r\n");

	src_saved = src;
	printf("src: %s\r\n", src);
	while ((ptr = acl_mystrtok(&src, " \t!'")) != NULL)
	{
		printf("--------------------------------------\r\n");
		printf("ptr: |%s|\r\n", ptr);
		printf("src: |%s|\r\n", src);
		printf("src_saved: |%s|\r\n", src_saved);
	}

	acl_myfree(src_saved);

	printf("----------------------------------------------\r\n");

	buf = acl_vstring_alloc(1);
	acl_vstring_sprintf(buf, "%*lu, s: %s, n1: %20u, n2: %20lu, n3: %20llu\n",
		(int) sizeof(unsigned long) * 4, (unsigned long) getpid(),
		s, n1, n2, n3);
	printf("buf: %s\r\n", acl_vstring_str(buf));
	acl_vstring_free(buf);

	printf("Enter any key to continue ...\r\n");
	getchar();

	test_quote_split();

	return 0;
}
