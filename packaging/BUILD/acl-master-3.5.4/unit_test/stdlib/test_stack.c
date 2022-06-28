#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

#include "test_stdlib.h"

typedef struct MY_TYPE {
	char  name[256];
} MY_TYPE;

ACL_STACK *__stack = NULL;

static void __stack_init(void)
{
	if (__stack != NULL)
		return;
	__stack = acl_stack_create(100);
}

int test_stack_bat_add(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	MY_TYPE *t;
	int   i;

	__stack_init();

	for (i = 0; i < 25; i++) {
		t = acl_mymalloc(sizeof(MY_TYPE));
		snprintf(t->name, sizeof(t->name), "name:%d", i);
		acl_stack_append(__stack, t);
	}
	return (0);
}

int test_stack_walk(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	const char *myname = "test_stack_walk";
	ACL_ITER iter;
	MY_TYPE *t;
	int   i = 0;

	__stack_init();

	acl_foreach(iter, __stack) {
		t = (MY_TYPE*) iter.data;
		printf("%s: %d %s\r\n", myname, i++, t->name);
	}
	printf("%s: total is %d\r\n", myname, i);
	return (0);
}

int test_stack_pop(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	const char *myname = "test_stack_pop";
	MY_TYPE *t;
	int   i = 0;

	__stack_init();

	while (1) {
		t = acl_stack_pop(__stack);
		if (t == NULL)
			break;
		printf("%s: %d %s\r\n", myname, i++, t->name);
		acl_myfree(t);
	}

	printf("%s: total is %d\r\n", myname, i);
	return (0);
}
