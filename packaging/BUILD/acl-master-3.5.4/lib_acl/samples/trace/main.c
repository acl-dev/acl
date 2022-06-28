#include "lib_acl.h"
#include <execinfo.h>
#include <stdio.h>
#include <string.h>
#include "sigsegv.h"

static void stack3(void)
{
	const char *file = "trace.txt";
	void *buf[10];
	size_t n, i;
	char **s;
	char *m = NULL;

	acl_trace_save(file);

	n = backtrace(buf, 10);
	s = backtrace_symbols(buf, n);

	for (i = 0; i < n; i++)
		printf(">>%s\n", s[i]);

	free(s);
	strcpy(m, "hello");
}

static void stack2(void)
{
	stack3();
}

static void stack1(void)
{
	stack2();
}

static void trace(void)
{
	stack1();
}

int main(void)
{
	setup_sigsegv();
	trace();
	return (0);
}
