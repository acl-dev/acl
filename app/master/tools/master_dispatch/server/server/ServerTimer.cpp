#include "stdafx.h"
#include "server/ServerManager.h"
#include "server/ServerTimer.h"

void ServerTimer::destroy()
{
	delete this;
}

// 璇ュ畾鏃跺櫒鐨勫洖璋冨嚱鏁拌繍琛屽湪涓荤嚎绋嬬嚎绋嬬┖闂�
void ServerTimer::timer_callback(unsigned int)
{
	// 鍦ㄥ畾鏃跺櫒涓畾鏃剁粺璁℃湇鍔＄鐨勮礋杞芥儏鍐�
	ServerManager::get_instance().buildStatus();
}
