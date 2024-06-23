#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <ucontext.h>
#include <libunwind.h>
#include "libmm.h"

static void stacktrace(void)
{
	unw_cursor_t cursor;
	unw_word_t off, pc;
	char name[128];
	int  ret;
	ucontext_t context;

	if (getcontext(&context) < 0) {
		printf("getcontext error\r\n");
		return;
	}

	ret = unw_init_local(&cursor, (unw_context_t*) &context);
	if (ret != 0) {
		printf("unw_init_local error, ret=%d\r\n", ret);
		return;
	}

	while (unw_step(&cursor) > 0) {
		ret = unw_get_proc_name(&cursor, name, sizeof(name), &off);
		if (ret != 0) {
			printf("unw_get_proc_name error =%d\n", ret);
		} else {
			unw_get_reg(&cursor, UNW_REG_IP, &pc);
			printf("0x%lx:(%s()+0x%lx)\n", pc, name, off);
		}
	}
}

static size_t total_size = 0;
static bool __check = false;
static pthread_mutex_t __lock;

static pthread_once_t once_control = PTHREAD_ONCE_INIT;

static void once_init(void)
{
	pthread_mutex_init(&__lock, NULL);
}

void *malloc(size_t size)
{
	void *ptr;

	pthread_once(&once_control, once_init);

	if (size == (size_t) -1) {
		__check = true;
		pthread_mutex_lock(&__lock);
		total_size = 0;
		pthread_mutex_unlock(&__lock);
		printf("check started!\r\n");
		return NULL;
	}
	if (size == 0) {
		__check = false;
		printf("check stopped!\r\n");
		return NULL;
	}

	ptr = mymalloc(size);
	if (ptr && __check) {
		if (size == 8208) {
			//abort();
		}

		if (1) {
			printf("---------------------------------------\r\n");
			stacktrace();
		}

		pthread_mutex_lock(&__lock);
		total_size += size;
		pthread_mutex_unlock(&__lock);
		printf(">>>malloc ptr=%p, size=%zd, total=%zd\r\n",
			ptr, size, total_size);
		printf("\r\n");
	}

	return ptr;
}
