
#include "lib_acl.h"
#include "stdlib/acl_avl.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "test_stdlib.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

avl_tree_t *__avl_tree = NULL;

typedef struct MY_TYPE {
	char  name[256];
	char  value[256];
	avl_node_t  node;
} MY_TYPE;

static int compare_fn(const void *v1, const void *v2)
{
	const MY_TYPE *m1 = (const MY_TYPE*) v1, *m2 = (const MY_TYPE*) v2;

	return (strcmp(m1->name, m2->name));
}

int test_avl_create(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	const char *myname = "test_avl_create";

	if (__avl_tree != NULL) {
		printf("%s: should not be called more than once!\n", myname);
		return (-1);
	}

	__avl_tree = (avl_tree_t *) acl_mycalloc(1, sizeof(avl_tree_t));
	assert(__avl_tree);
	avl_create(__avl_tree, compare_fn, sizeof(MY_TYPE), offsetof(MY_TYPE, node));

	return (0);
}

int test_avl_add_bat(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	MY_TYPE *pm, m;
	int   i, n = 0;

	for (i = 0; i < 100; i++) {
		snprintf(m.name, sizeof(m.name), "%d", i);
		pm = (MY_TYPE*) avl_find(__avl_tree, &m, NULL);
		if (pm != NULL) {
			printf(">>key(%s) already exist, value(%s)\r\n",
				pm->name, pm->value);
			continue;
		}

		pm = (MY_TYPE*) acl_mycalloc(1, sizeof(MY_TYPE));
		snprintf(pm->name, sizeof(pm->name), "%d", i);
		snprintf(pm->value, sizeof(pm->value), "value(%d)", i);
		avl_add(__avl_tree, pm);
		printf(">>add one, key(%s), value(%s)\r\n", pm->name, pm->value);
		n++;
	} 

	return (0);
}

int test_avl_walk(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	MY_TYPE *next;
	int   n = 0;

	next = (MY_TYPE*) avl_first(__avl_tree);
	while (next) {
		printf(">>name(%s), value(%s)\r\n", next->name, next->value);
		next = (MY_TYPE*) AVL_NEXT(__avl_tree, next);
		n++;
	}

	printf(">>ok, total count is %d\r\n", n);
	return (0);
}

int test_avl_find(AUT_LINE *test_line, void *arg acl_unused)
{
	MY_TYPE m, *pm;
	const char *pname;

	AUT_SET_STR(test_line, "name", pname);
	snprintf(m.name, sizeof(m.name), "%s", pname);

	pm = (MY_TYPE*) avl_find(__avl_tree, &m, NULL);
	if (pm) {
		printf(">>ok, find it, %s, %s\r\n", m.name, pm->value);
		return (0);
	} else {
		printf(">>not find it, name=%s\r\n", m.name);
		return (-1);
	}
}

int test_avl_add(AUT_LINE *test_line, void *arg acl_unused)
{
	MY_TYPE m, *pm;
	const char *pname, *pvalue;

	AUT_SET_STR(test_line, "name", pname);
	AUT_SET_STR(test_line, "value", pvalue);
	snprintf(m.name, sizeof(m.name), "%s", pname);
	snprintf(m.value, sizeof(m.value), "%s", pvalue);

	pm = (MY_TYPE*) avl_find(__avl_tree, &m, NULL);
	if (pm) {
		printf(">>err, already exist %s, %s\r\n", m.name, pm->value);
		return (-1);
	} else {
		pm = (MY_TYPE*) acl_mycalloc(1, sizeof(MY_TYPE));
		snprintf(pm->name, sizeof(pm->name), "%s", pname);
		snprintf(pm->value, sizeof(pm->value), "value(%s)", pvalue);
		avl_add(__avl_tree, pm);
		printf(">>ok, added, name=%s\r\n", m.name);
		return (0);
	}
}

int test_avl_delete(AUT_LINE *test_line, void *arg acl_unused)
{
	MY_TYPE m, *pm;
	const char *pname;

	AUT_SET_STR(test_line, "name", pname);
	snprintf(m.name, sizeof(m.name), "%s", pname);

	pm = (MY_TYPE*) avl_find(__avl_tree, &m, NULL);
	if (pm == NULL) {
		printf(">>not find %s\r\n", m.name);
		return (-1);
	}

	avl_remove(__avl_tree, pm);
	printf(">>ok, delete %s\r\n", m.name);
	return (0);
}
