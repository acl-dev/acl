#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"

static int __max_loop = 100;
static int __max_fiber = 10;
static int __stack_size = 64000;

/* 协程处理入口函数 */
static void fiber_main(ACL_FIBER *fiber, void *ctx acl_unused)
{
	int  i;

	/* 两种方式均可以获得当前的协程号 */
	assert(acl_fiber_self() == acl_fiber_id(fiber));

	for (i = 0; i < __max_loop; i++) {
		acl_fiber_yield(); /* 主动让出 CPU 给其它协程 */

		printf("fiber-%d\r\n", acl_fiber_self());
	}
}

int main(void)
{
	int   ch, i;

	/* 创建协程 */
	for (i = 0; i < __max_fiber; i++)
		acl_fiber_create(fiber_main, NULL, __stack_size);

	printf("---- begin schedule fibers now ----\r\n");

	/* 循环调度所有协程，直至所有协程退出 */
	acl_fiber_schedule();

	printf("---- all fibers exit ----\r\n");
	return 0;
}
