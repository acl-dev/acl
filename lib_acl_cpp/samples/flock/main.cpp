#include "acl_cpp/stdlib/locker.hpp"
#include "acl_cpp/stream/fstream.hpp"
#include <stdio.h>
#include <fcntl.h>

int main(void)
{
	acl::fstream fp;

	if (fp.open("test.lock", O_RDWR, 0600) == false) {
		printf("open file error\n");
		getchar();
		return (1);
	}

	acl::locker locker;

	if (locker.open(fp.file_handle()) == false) {
		printf("open lock error\n");
		getchar();
		return (1);
	}
	if (locker.lock() == true) {
		printf("lock ok\n");
	} else {
		printf("lock error\n");
	}

	printf("enter any key to exit\n");
	getchar();
	return (0);
}

