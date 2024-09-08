#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"

/* 协程处理入口函数 */
static void fiber_main(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	printf("---fiber is running---\r\n");
	acl_fiber_delay(2000);
	printf("---fiber is exiting---\r\n");
}

int main(void)
{
	acl_fiber_schedule_init(1);

	printf("---begin create one fiber----\r\n");
	acl_fiber_create(fiber_main, NULL, 64000);

	printf("---- all fibers exit ----\r\n");
	return 0;
}
