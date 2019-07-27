#include <assert.h>
#include <iostream>
#include "acl_cpp/lib_acl.hpp"
#include "fiber/lib_fiber.hpp"

static void fiber_main(int max_loop)
{
	for (int i = 0; i < max_loop; i++)
	{
		acl::fiber::yield(); // 涓诲姩璁╁嚭 CPU 缁欏叾瀹冨崗绋�
		std::cout << "fiber-" << acl::fiber::self() << std::endl;
	}
}

int main(void)
{
	int i, max_fiber = 10, max_loop = 10;
	
	for (i = 0; i < max_fiber; i++)
	{
		go[=] { // 閲囩敤 c++11 鐨� lambad 琛ㄨ揪寮忔柟寮忓垱寤哄崗绋�
			fiber_main(max_loop); // 杩涘叆鍗忕▼澶勭悊鍑芥暟
		};
	}

	std::cout << "---- begin schedule fibers now ----" << std::endl;
	// 寰幆璋冨害鎵€鏈夊崗绋嬶紝鐩磋嚦鎵€鏈夊崗绋嬮€€鍑�
	acl::fiber::schedule();
	std::cout << "---- all fibers exit ----" << std::endl;

	return 0;
}
