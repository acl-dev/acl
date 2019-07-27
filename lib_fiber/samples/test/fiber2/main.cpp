#include <assert.h>
#include <iostream>
#include "fiber/lib_fiber.hpp"

class myfiber : public acl::fiber
{
public:
	myfiber(int max_loop) : max_loop_(max_loop) {}

protected:
	// @override 瀹炵幇鍩虹被绾櫄鍑芥暟
	void run(void)
	{
		// 涓ょ鏂瑰紡鍧囧彲浠ヨ幏寰楀綋鍓嶇殑鍗忕▼鍙�
		assert(get_id() == acl::fiber::self());

		for (int i = 0; i < max_loop_; i++)
		{
			acl::fiber::yield(); // 涓诲姩璁╁嚭 CPU 缁欏叾瀹冨崗绋�
			std::cout << "fiber-" << acl::fiber::self() << std::endl;
		}

		delete this; // 鍥犱负鏄姩鎬佸垱寤虹殑锛屾墍浠ラ渶鑷姩閿€姣�
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
		acl::fiber* fb = new myfiber(max_loop); // 鍒涘缓鍗忕▼
		fb->start(); // 鍚姩鍗忕▼
	}

	std::cout << "---- begin schedule fibers now ----" << std::endl;
	// 寰幆璋冨害鎵€鏈夊崗绋嬶紝鐩磋嚦鎵€鏈夊崗绋嬮€€鍑�
	acl::fiber::schedule();
	std::cout << "---- all fibers exit ----" << std::endl;

	return 0;
}
