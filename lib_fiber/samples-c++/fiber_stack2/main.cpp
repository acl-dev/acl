#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class myfiber : public acl::fiber
{
public:
	myfiber(acl::fiber_tbox<bool>& box) : box_(box) {}
	~myfiber(void) {}

private:
	acl::fiber_tbox<bool>& box_;

	// @override
	void run(void)
	{
		printf("fiber-%d-%d running\r\n", get_id(), acl::fiber::self());
		func1();

		delete this;
	}

	void func1(void)
	{
		func2();
	}

	void func2(void)
	{
		func3();
	}

	void func3(void)
	{
		box_.pop();
	}
};

class checker : public acl::fiber
{
public:
	checker(acl::fiber_tbox<bool>& box, acl::fiber* fb, int n)
	: box_(box), fb_(fb), count_(n) {}
	~checker(void) {}

private:
	acl::fiber_tbox<bool>& box_;
	acl::fiber* fb_;
	int count_;

	void run(void)
	{
		for (int i = 0; i < count_; i++) {
			std::vector<acl::fiber_frame> stack;
			acl::fiber::stacktrace(*fb_, stack, 50);
			show_stack(stack);
			printf("\r\n");

			sleep(2);
		}

		box_.push(NULL);
		delete this;
	}

	void show_stack(const std::vector<acl::fiber_frame>& stack)
	{
		for (std::vector<acl::fiber_frame>::const_iterator
			cit = stack.begin(); cit != stack.end(); ++cit) {
			printf("0x%lx(%s)+0x%lx\r\n",
				(*cit).pc, (*cit).func.c_str(), (*cit).off);
		}
	}
};

int main(void)
{
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::fiber_tbox<bool> box;
	acl::fiber* f = new myfiber(box);
	f->start();

	acl::fiber* f2 = new checker(box, f, 10);
	f2->start();

	acl::fiber::schedule();
	return 0;
}
