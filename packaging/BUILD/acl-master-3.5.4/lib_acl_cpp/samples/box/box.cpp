#include "stdafx.h"

#define BOX	acl::mbox
//#define BOX	acl::tbox
//#define BOX	acl::tbox_array

class producer : public acl::thread
{
public:
	producer(BOX<int>& box, int max) : box_(box), max_(max) {}
	~producer(void) {}

protected:
	void* run(void)
	{
		for (int i = 0; i < max_; i++) {
			int* n = new int;
			*n = i;
			box_.push(n);
		}

		return NULL;
	}

private:
	BOX<int>& box_;
	int max_;
};

int main(void)
{
	int max = 50000000;
	BOX<int> box;

	producer thr(box, max);
	thr.start();

	for (int i = 0; i < max; i++) {
		int* n = box.pop();
		assert(*n == i);
		delete n;
	}

	printf("All over, max=%d\r\n", max);
	thr.wait();
	return 0;
}
