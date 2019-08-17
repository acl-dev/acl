#include "stdafx.h"
#include "FiberSleep.h"

CFiberSleep::CFiberSleep(void)
{
}

CFiberSleep::~CFiberSleep(void)
{
}

void CFiberSleep::run(void)
{
	printf("timer fiber-%d created\r\n", acl::fiber::self());
	for (int i = 0; i < 5; i++) {
		acl::fiber::delay(1000);
		printf("fiber-%d wakeup\r\n", acl::fiber::self());
	}

	printf("sleep fiber exit now\r\n");
	delete this;
}