#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif

#define	NO_CPP_DEMANGLE

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <execinfo.h>
#ifndef NO_CPP_DEMANGLE
#include <cxxabi.h>
#endif
#include "sigsegv.h"

#if defined(REG_RIP)
# define SIGSEGV_STACK_IA64
# define REGFORMAT "%016lx"
#elif defined(REG_EIP)
# define SIGSEGV_STACK_X86
# define REGFORMAT "%08x"
#else
# define SIGSEGV_STACK_GENERIC
# define REGFORMAT "%x"
#endif

static void signal_segv(int signum, siginfo_t* info, void*ptr)
{
	static const char *si_codes[3] = {"", "SEGV_MAPERR", "SEGV_ACCERR"};

	size_t i;
	ucontext_t *ucontext = (ucontext_t*)ptr;

#if defined(SIGSEGV_STACK_X86) || defined(SIGSEGV_STACK_IA64)
	int f = 0;
	Dl_info di;
	void **bp = 0;
	void *ip = 0;
#else
	void *bt[20];
	char **strings;
	size_t sz;
#endif

	fprintf(stderr, "Segmentation Fault!\n");
	fprintf(stderr, "info.si_signo = %d\n", signum);
	fprintf(stderr, "info.si_errno = %d\n", info->si_errno);
	fprintf(stderr, "info.si_code  = %d (%s)\n", info->si_code,
		si_codes[info->si_code]);
	fprintf(stderr, "info.si_addr  = %p\n", info->si_addr);
	for(i = 0; i < NGREG; i++)
		fprintf(stderr, "reg[%02d]       = 0x" REGFORMAT "\n",
			(int) i, (unsigned long) ucontext->uc_mcontext.gregs[i]);

#if defined(SIGSEGV_STACK_X86) || defined(SIGSEGV_STACK_IA64)
# if defined(SIGSEGV_STACK_IA64)
	ip = (void*)ucontext->uc_mcontext.gregs[REG_RIP];
	bp = (void**)ucontext->uc_mcontext.gregs[REG_RBP];
# elif defined(SIGSEGV_STACK_X86)
	ip = (void*)ucontext->uc_mcontext.gregs[REG_EIP];
	bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP];
# endif

	fprintf(stderr, "Stack trace:\n");
	while(bp && ip) {
		const char *symname;
		if(!dladdr(ip, &di))
			break;

		symname = di.dli_sname;
#ifndef NO_CPP_DEMANGLE
		int status;
		char *tmp = __cxa_demangle(symname, NULL, 0, &status);

		if(status == 0 && tmp)
			symname = tmp;
#endif

		fprintf(stderr, "%2d: %p <%s+%u> (%s)\n",
				++f,
				ip,
				symname,
				(unsigned)(((char*) ip) - ((char*) di.dli_saddr)),
				di.dli_fname);

#ifndef NO_CPP_DEMANGLE
		if(tmp)
			free(tmp);
#endif

		if(di.dli_sname && !strcmp(di.dli_sname, "main"))
			break;

		ip = bp[1];
		bp = (void**)bp[0];
	}
#else
	fprintf(stderr, "Stack trace (non-dedicated):\n");
	sz = backtrace(bt, 20);
	strings = backtrace_symbols(bt, sz);

	for(i = 0; i < sz; ++i)
		fprintf(stderr, "%s\n", strings[i]);
#endif
	fprintf(stderr, "End of stack trace\n");
	exit (-1);
}

int setup_sigsegv()
{
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = signal_segv;
	action.sa_flags = SA_SIGINFO;
	if(sigaction(SIGSEGV, &action, NULL) < 0) {
		perror("sigaction");
		return 0;
	}

	return 1;
}
