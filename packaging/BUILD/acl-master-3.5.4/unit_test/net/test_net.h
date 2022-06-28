
#ifndef	__TEST_NET_INCLUDE_H__
#define	__TEST_NET_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

/* test_net.c: register all test functions */
void test_net_register(void);

/* in test_connect.c */
int test_connect(AUT_LINE *test_line, void *arg);

/* in test_net_misc.c */
int test_mask_addr(AUT_LINE *test_line, void *arg);

#ifdef	__cplusplus
}
#endif

#endif

