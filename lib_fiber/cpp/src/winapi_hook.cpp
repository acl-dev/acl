#include "stdafx.hpp"
#include "winapi_hook.hpp"

#if (defined(_WIN32) || defined(_WIN64)) && _MSC_VER >= 1929
#include "detours/detours.h"

typedef unsigned long nfds_t;
#include "../../c/src/hook/hook.h"

socket_fn    __socket    = socket;
listen_fn    __listen    = listen;
close_fn     __close     = closesocket;
accept_fn    __accept    = accept;
connect_fn   __connect   = connect;
recv_fn      __recv      = recv;
recvfrom_fn  __recvfrom  = recvfrom;
send_fn      __send      = send;
sendto_fn    __sendto    = sendto;
select_fn    __select    = select;
poll_fn      __poll      = WSAPoll;

#define HOOK_API(from, to, action) do { \
	LONG ret = DetourAttach(&from, to); \
	if (ret != 0) { \
		logger("DetourAttach %s failed %s", #from, acl::last_serror()); \
		return false; \
	} \
	action(&from); \
} while (0)

bool winapi_hook(void) {
	static bool __called = false;
	if (__called) {
		return true;
	}

	__called = true;

	//DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	HOOK_API(__socket, &acl_fiber_socket, set_socket_fn);
	HOOK_API(__close, acl_fiber_close, set_close_fn);
	HOOK_API(__listen, acl_fiber_listen, set_listen_fn);
	HOOK_API(__accept, acl_fiber_accept, set_accept_fn);
	HOOK_API(__connect, acl_fiber_connect, set_connect_fn);
	HOOK_API(__recv, acl_fiber_recv, set_recv_fn);
	HOOK_API(__recvfrom, acl_fiber_recvfrom, set_recvfrom_fn);
	HOOK_API(__send, acl_fiber_send, set_send_fn);
	HOOK_API(__sendto, acl_fiber_sendto, set_sendto_fn);
	HOOK_API(__poll, acl_fiber_poll, set_poll_fn);
	HOOK_API(__select, acl_fiber_select, set_select_fn);

	DetourTransactionCommit();
	return true;
}

#else

bool winapi_hook(void) {
	return true;
}

#endif
