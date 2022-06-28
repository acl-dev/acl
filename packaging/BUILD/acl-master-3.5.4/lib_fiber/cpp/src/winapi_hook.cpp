#include "stdafx.hpp"
#include "winapi_hook.hpp"

#if (defined(_WIN32) || defined(_WIN64)) && _MSC_VER >= 1920
#include "detours/detours.h"

typedef unsigned long nfds_t;
#include "../../c/src/hook/hook.h"
#include "../../c/src/common/pthread_patch.h"

socket_fn     __socket     = socket;
listen_fn     __listen     = listen;
close_fn      __close      = closesocket;
accept_fn     __accept     = accept;
connect_fn    __connect    = connect;
recv_fn       __recv       = recv;
recvfrom_fn   __recvfrom   = recvfrom;
send_fn       __send       = send;
sendto_fn     __sendto     = sendto;
select_fn	  __select     = select;

poll_fn       __poll       = WSAPoll;
WSARecv_fn    __WSARecv    = WSARecv;
WSAAccept_fn  __WSAAccept  = WSAAccept;

getaddrinfo_fn   __getaddrinfo   = getaddrinfo;
freeaddrinfo_fn  __freeaddrinfo  = freeaddrinfo;
gethostbyname_fn __gethostbyname = gethostbyname;

#define HOOK_API(from, to, action) do { \
	LONG ret = DetourAttach(&from, to); \
	if (ret != 0) { \
		logger("DetourAttach %s failed %s", #from, acl::last_serror()); \
		return; \
	} else { \
		action(&from); \
	} \
} while (0)

static bool __hook_ok = false;

static void winapi_hook_once(void) {
	//DetourIsHelperProcess();
	DetourRestoreAfterWith();
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
	HOOK_API(__getaddrinfo, acl_fiber_getaddrinfo, set_getaddrinfo_fn);
	HOOK_API(__freeaddrinfo, acl_fiber_freeaddrinfo, set_freeaddrinfo_fn);
	HOOK_API(__gethostbyname, acl_fiber_gethostbyname, set_gethostbyname_fn);

#ifdef SYS_WSA_API
	HOOK_API(__WSARecv, acl_fiber_WSARecv, set_WSARecv_fn);
	HOOK_API(__WSAAccept, acl_fiber_WSAAccept, set_WSAAccept_fn);
#endif

	DetourTransactionCommit();
	__hook_ok = true;
}

static pthread_once_t __once = PTHREAD_ONCE_INIT;

bool winapi_hook(void) {
	if (pthread_once(&__once, winapi_hook_once) != 0) {
		return false;
	}

#if 0
	// just test socket hook
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		printf("create socket error\r\n");
		return false;
	} else {
		printf("create socket ok\r\n");
		char buf[8192];
		//int ret = recv(s, buf, sizeof(buf), 0);
		int ret = acl_socket_read(s, buf, sizeof(buf), -1, NULL, NULL);
		printf(">>>recv ret=%d\r\n", ret);
	}
#endif

	return __hook_ok;
}

#else

bool winapi_hook(void) {
	return true;
}

#endif
