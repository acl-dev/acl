
#ifndef	__TEST_STDLIB_INCLUDE_H__
#define	__TEST_STDLIB_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

/* test_stdlib.c: register all test functions */
void test_stdlib_register(void);

/* test_mylock.c */
int test_mylock_unlock(AUT_LINE *test_line, void *arg_unused);
int test_mylock_excl(AUT_LINE *test_line, void *arg_unused);
int test_mylock_nowait(AUT_LINE *test_line, void *arg_unused);
int test_mylock_shared(AUT_LINE *test_line, void *arg_unused);

/* test_ring.c */
int test_ring(AUT_LINE *test_line, void *arg_unused);

/* test_htable.c */
int test_htable_create(AUT_LINE *test_line, void *arg_unused);
int test_htable_stat(AUT_LINE *test_line, void *arg_unused);
int test_htable_free(AUT_LINE *test_line, void *arg_unused);
int test_htable_find(AUT_LINE *test_line, void *arg_unused);
int test_htable_rwlock(AUT_LINE *test_line, void *arg_unused);

/* in test_malloc.c */
int test_fatal_free(AUT_LINE *test_line acl_unused, void *arg acl_unused);
int test_fatal_malloc(AUT_LINE *test_line acl_unused, void *arg acl_unused);

/* in test_avl.c */
int test_avl_create(AUT_LINE *test_line acl_unused, void *arg acl_unused);
int test_avl_add_bat(AUT_LINE *test_line acl_unused, void *arg acl_unused);
int test_avl_walk(AUT_LINE *test_line acl_unused, void *arg acl_unused);
int test_avl_find(AUT_LINE *test_line, void *arg acl_unused);
int test_avl_add(AUT_LINE *test_line, void *arg acl_unused);
int test_avl_delete(AUT_LINE *test_line, void *arg acl_unused);

/* in test_stack.c */
int test_stack_bat_add(AUT_LINE *test_line, void *arg);
int test_stack_walk(AUT_LINE *test_line, void *arg);
int test_stack_pop(AUT_LINE *test_line, void *arg);

/* in test_vstream.c */
int test_file_vstream(AUT_LINE *test_line, void *arg);

/* in test_string.c */
int test_strrncasecmp(AUT_LINE *test_line, void *arg);
int test_strrncmp(AUT_LINE *test_line, void *arg);
int test_x64toa(AUT_LINE *test_line, void *arg);
int test_strcasestr(AUT_LINE *test_line, void *arg);

#ifdef	__cplusplus
}
#endif

#endif

