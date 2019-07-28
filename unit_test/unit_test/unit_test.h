
#ifndef	__UNIT_TEST_INCLUDE_H__
#define	__UNIT_TEST_INCLUDE_H__

#include "lib_acl.h"

/* in unit_test.c: register all test functionis */
void test_unit_register(void);

/* in unit_test_fn.c */
int test_unit_param(AUT_LINE *test_line, void *arg_unused);
int test_unit_loop(AUT_LINE *test_line, void *arg_unused);

#endif

