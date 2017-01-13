#include <assert.h>
#include <iostream>
#include "acl_cpp/lib_acl.hpp"
#include "fiber/lib_fiber.hpp"

static void fiber_main(int max_loop)
{
	for (int i = 0; i < max_loop; i++)
	{
		acl::fiber::yield(); // 主动让出 CPU 给其它协程
		std::cout << "fiber-" << acl::fiber::self() << std::endl;
	}
}

int main(void)
{
	int i, max_fiber = 10, max_loop = 10;
	
	for (i = 0; i < max_fiber; i++)
	{
		go[=] { // 采用 c++11 的 lambad 表达式方式创建协程
			fiber_main(max_loop); // 进入协程处理函数
		};
	}

	std::cout << "---- begin schedule fibers now ----" << std::endl;
	// 循环调度所有协程，直至所有协程退出
	acl::fiber::schedule();
	std::cout << "---- all fibers exit ----" << std::endl;

	return 0;
}
