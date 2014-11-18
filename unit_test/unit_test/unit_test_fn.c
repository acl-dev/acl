#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#include "unit_test.h"

int test_unit_param(AUT_LINE *test_line, void *arg_unused)
{
	char  myname[] = "test_unit_param";
	const char *account, *sender;
	int   number;

	arg_unused = arg_unused;
	AUT_SET_STR(test_line, "account", account);
	AUT_SET_STR(test_line, "sender", sender);
	AUT_SET_INT(test_line, "number", number);

	printf("%s: account=%s, sender=%s, number=%d\n",
			myname, account, sender, number);

	return (0);
}

int test_unit_loop(AUT_LINE *test_line, void *arg_unused)
{
	int   count, i;

	arg_unused = arg_unused;

	AUT_SET_INT(test_line, "count", count);
	if (count <= 0)
		count = 1;

	for (i = 0; i < count; i++) {}

	return (0);
}
