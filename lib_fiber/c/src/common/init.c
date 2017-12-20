#include "stdafx.h"
#include <pthread.h>

#include "init.h"

static pthread_t var_main_tid = (pthread_t) -1;

void lib_init(void) __attribute__ ((constructor));

void lib_init(void)
{
	static int __have_inited = 0;

	if (__have_inited)
		return;
	__have_inited = 1;
	var_main_tid = pthread_self();
}

unsigned long main_thread_self(void)
{
	return ((unsigned long) var_main_tid);
}
