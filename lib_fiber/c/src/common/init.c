#include "stdafx.h"
#include "pthread_patch.h"
#include "init.h"

static long var_main_tid = (unsigned long) -1;

#ifdef __GNUC__
void lib_init(void) __attribute__ ((constructor));
#endif

void lib_init(void)
{
	static int __have_inited = 0;

	if (__have_inited) {
		return;
	}
	__have_inited = 1;
	var_main_tid = thread_self();
}

long main_thread_self(void)
{
	return var_main_tid;
}
