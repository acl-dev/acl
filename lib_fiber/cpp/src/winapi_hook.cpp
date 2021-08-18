#include "stdafx.hpp"
#include "detours/detours.h"
#include "winapi_hook.hpp"

#if (defined(_WIN32) || defined(_WIN64)) && _MSC_VER >= 1911

SOCKET_FN    __socket    = socket;
LISTEN_FN    __listen    = listen;
CLOSE_FN     __close     = closesocket;
ACCEPT_FN    __accept    = accept;
CONNECT_FN   __connect   = connect;
RECV_FN      __recv      = recv;
RECVFROM_FN  __recvfrom  = recvfrom;
SEND_FN      __send      = send;
SENDTO_FN    __sendto    = sendto;
SELECT_FN    __select    = select;
POLL_FN      __poll      = WSAPoll;

#define HOOK_API(from, to) do { \
	LONG ret = DetourAttach(from, to); \
	if (ret != 0) { \
		logger("DetourAttach %s failed %s", #from, acl::last_serror()); \
		return false; \
	} \
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

	HOOK_API(&(PVOID&) __socket, &acl_fiber_socket);
	HOOK_API(&(PVOID&) __listen, acl_fiber_listen);
	HOOK_API(&(PVOID&) __close, acl_fiber_close);
	HOOK_API(&(PVOID&) __accept, acl_fiber_accept);
	HOOK_API(&(PVOID&) __connect, acl_fiber_connect);
	HOOK_API(&(PVOID&) __recv, acl_fiber_recv);
	HOOK_API(&(PVOID&) __recvfrom, acl_fiber_recvfrom);
	HOOK_API(&(PVOID&) __send, acl_fiber_send);
	HOOK_API(&(PVOID&) __sendto, acl_fiber_sendto);
	HOOK_API(&(PVOID&) __select, acl_fiber_select);
	HOOK_API(&(PVOID&) __poll, acl_fiber_poll);
	DetourTransactionCommit();
	return true;
}

#else

bool winapi_hook(void) {
	return true;
}

#endif