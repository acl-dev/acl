#include "http.h"

int main(int argc, char *argv[])
{
	http_async(argc, argv);
#ifdef WIN32
	printf("enter any key to exit ...\r\n");
	getchar();
#endif

	return 0;
}

