#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#ifdef	ACL_UNIX
#include <signal.h>
#endif
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstream.h"
#include "thread/acl_pthread.h"
#include "init/acl_init.h"

#endif /* ACL_PREPARE_COMPILE */

#if  defined(ACL_WINDOWS)
#pragma comment(lib,"ws2_32")
#pragma comment(lib, "wsock32")
#endif

#ifdef ACL_WINDOWS
#include <Tlhelp32.h>
#endif

#include "init.h"

static char *version = "3.5.1-5 20200627";

const char *acl_version(void)
{
	return version;
}

#ifdef ACL_UNIX
static acl_pthread_t acl_var_main_tid = (acl_pthread_t) -1;
#elif defined(ACL_WINDOWS)
static unsigned long acl_var_main_tid = (unsigned long) -1;
#else
#error "Unknown OS"
#endif

#ifdef	ACL_UNIX
void acl_lib_init(void) __attribute__ ((constructor));
#endif

void acl_lib_init(void)
{
	static int __have_inited = 0;

	if (__have_inited)
		return;
	__have_inited = 1;
#ifdef ACL_UNIX
	signal(SIGPIPE, SIG_IGN);
#elif  defined(ACL_WINDOWS)
	acl_socket_init();
	acl_vstream_init();
#endif
	acl_var_main_tid = acl_pthread_self();
}

#ifdef	ACL_UNIX
void acl_lib_end(void) __attribute__ ((destructor));
#endif

void acl_lib_end(void)
{
	static int __have_ended = 0;

	if (__have_ended)
		return;
	__have_ended = 1;
#if  defined(ACL_WINDOWS)
	acl_socket_end();
	acl_pthread_end();
#endif
}

int __acl_var_use_poll = 1;

void acl_poll_prefered(int yesno)
{
	__acl_var_use_poll = yesno;
}

#ifdef ACL_WINDOWS
static acl_pthread_once_t __once_control = ACL_PTHREAD_ONCE_INIT;

static void get_main_thread_id(void)
{
	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	THREADENTRY32 th32;
	DWORD currentPID;
	BOOL  bOk;

	if (hThreadSnap == INVALID_HANDLE_VALUE)
		return;
	currentPID = GetCurrentProcessId();
	th32.dwSize = sizeof(THREADENTRY32);

	for (bOk = Thread32First(hThreadSnap, &th32); bOk;
		bOk = Thread32Next(hThreadSnap, &th32))
	{
		if (th32.th32OwnerProcessID == currentPID) {
			acl_var_main_tid = th32.th32ThreadID;
			break;
		}
	}
}
#endif

unsigned long acl_main_thread_self()
{
#ifdef ACL_UNIX
	return ((unsigned long) acl_var_main_tid);
#elif defined(ACL_WINDOWS)
	if (acl_var_main_tid == (unsigned long) -1)
		acl_pthread_once(&__once_control, get_main_thread_id);
	return (acl_var_main_tid);
#else
#error "Unknown OS"
#endif
}
