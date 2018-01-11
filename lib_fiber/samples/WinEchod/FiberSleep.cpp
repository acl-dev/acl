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
	while (true)
	{
		acl::fiber::delay(1000);
		printf("fiber-%d wakeup\r\n", acl::fiber::self());
	}
}