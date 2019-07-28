#include "stdafx.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static acl::string __fpath("./tmp.dmp");
static off_t __max = 1000000;

static void* thread_main(void* arg)
{
	sleep(10);

	printf("thread started\r\n");
	char* ptr = (char*) arg;
	for (off_t i = 0; i < __max; i++)
	{
		int ch = *ptr++;
		if (i % 9999999 == 0)
			printf("get ch: %c\r\n", ch);
	}

	return NULL;
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -f file_path -n file_size -u [call munmap] -o get|set\r\n", procname);
}

int main(int argc, char* argv[])
{
	acl::string action("get");
	bool  unuse = false;
	int   ch;

	while ((ch = getopt(argc, argv, "hf:n:uo:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			__fpath = optarg;
			break;
		case 'n':
			__max = atol(optarg);
			break;
		case 'u':
			unuse = true;
			break;
		case 'o':
			action = optarg;
			break;
		default:
			break;
		}
	}

	int fd = open(__fpath.c_str(), O_RDWR | O_CREAT, 0600);
	if (fd == -1)
	{
		printf("open %s error\r\n", __fpath.c_str());
		return 1;
	}

	char* ptr = (char*) mmap(NULL, __max, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == NULL)
	{
		printf("mmap error %s\r\n", acl::last_serror());
		return 1;
	}

	printf("file size: %ld\r\n", __max);
	lseek(fd, __max, SEEK_SET);
	write(fd, "x", 1);
	close(fd);
	char* ptr_saved = ptr;

	printf("action: %s\r\n", action.c_str());

	if (action == "get")
	{
		for (off_t i = 0; i < __max; i++)
		{
			ch = *ptr++;
			if (i % 9999999 == 0)
				printf("ch: %d, %c, i: %ld\n", ch, ch, i);
		}
	}
	else if (action == "set")
	{
		acl_pthread_t tid;
		acl_pthread_create(&tid, NULL, thread_main, ptr);

		for (off_t i = 0; i < __max; i++)
		{
			if (i % 100 == 0)
				*ptr = '\n';
			else
				*ptr = 'x';
			if (i % 9999999 == 0)
				printf("set: %c\r\n", *ptr);
			ptr++;
		}
	}
	else
		printf("unknown action: %s\r\n", action.c_str());

	if (unuse)
		munmap(ptr_saved, __max);
	sleep(100);
	return 0;
}
