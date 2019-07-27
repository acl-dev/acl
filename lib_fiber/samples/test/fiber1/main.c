#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"

static int __max_loop = 100;
static int __max_fiber = 10;
static int __stack_size = 64000;

/* 鍗忕▼澶勭悊鍏ュ彛鍑芥暟 */
static void fiber_main(ACL_FIBER *fiber, void *ctx acl_unused)
{
	int  i;

	/* 涓ょ鏂瑰紡鍧囧彲浠ヨ幏寰楀綋鍓嶇殑鍗忕▼鍙� */
	assert(acl_fiber_self() == acl_fiber_id(fiber));

	for (i = 0; i < __max_loop; i++) {
		acl_fiber_yield(); /* 涓诲姩璁╁嚭 CPU 缁欏叾瀹冨崗绋� */

		printf("fiber-%d\r\n", acl_fiber_self());
	}
}

int main(void)
{
	int   ch, i;

	/* 鍒涘缓鍗忕▼ */
	for (i = 0; i < __max_fiber; i++)
		acl_fiber_create(fiber_main, NULL, __stack_size);

	printf("---- begin schedule fibers now ----\r\n");

	/* 寰幆璋冨害鎵€鏈夊崗绋嬶紝鐩磋嚦鎵€鏈夊崗绋嬮€€鍑� */
	acl_fiber_schedule();

	printf("---- all fibers exit ----\r\n");
	return 0;
}
