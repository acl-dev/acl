#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
	acl::fiber_tbox<int> box;

	go[&box] {
		for (int i = 0; i < 10000; i++) {
			int *n = new int(i);
			printf("fiber-%d: Push %d\r\n", acl::fiber::self(), *n);
			box.push(n);
			acl::fiber::delay(1000);
		}
	};


	go[&box] {
		while (true) {
			bool found;
			int *n = box.pop(1000, &found);
			if (n) {
				printf("fiber-%d: Got %d\r\n", acl::fiber::self(), *n);
				delete n;
			} else {
				printf("fiber-%d: Got timeout\r\n", acl::fiber::self());
			}
		}
	};

	acl::fiber::schedule();
	return 0;
}
