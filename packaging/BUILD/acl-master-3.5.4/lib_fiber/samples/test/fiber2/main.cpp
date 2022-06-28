#include <assert.h>
#include <iostream>
#include "fiber/lib_fiber.hpp"

class myfiber : public acl::fiber
{
public:
	myfiber(int max_loop) : max_loop_(max_loop) {}

protected:
	// @override 实现基类纯虚函数
	void run(void)
	{
		// 两种方式均可以获得当前的协程号
		assert(get_id() == acl::fiber::self());

		for (int i = 0; i < max_loop_; i++)
		{
			acl::fiber::yield(); // 主动让出 CPU 给其它协程
			std::cout << "fiber-" << acl::fiber::self() << std::endl;
		}

		delete this; // 因为是动态创建的，所以需自动销毁
	}

private:
	int max_loop_;

	~myfiber(void) {}
};

int main(void)
{
	int i, max_fiber = 10, max_loop = 10;
	
	for (i = 0; i < max_fiber; i++)
	{
		acl::fiber* fb = new myfiber(max_loop); // 创建协程
		fb->start(); // 启动协程
	}

	std::cout << "---- begin schedule fibers now ----" << std::endl;
	// 循环调度所有协程，直至所有协程退出
	acl::fiber::schedule();
	std::cout << "---- all fibers exit ----" << std::endl;

	return 0;
}
