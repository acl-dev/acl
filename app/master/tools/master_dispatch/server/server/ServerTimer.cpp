#include "stdafx.h"
#include "server/ServerManager.h"
#include "server/ServerTimer.h"

void ServerTimer::destroy()
{
	delete this;
}

// 该定时器的回调函数运行在主线程线程空间
void ServerTimer::timer_callback(unsigned int)
{
	// 在定时器中定时统计服务端的负载情况
	ServerManager::get_instance().buildStatus();
}
