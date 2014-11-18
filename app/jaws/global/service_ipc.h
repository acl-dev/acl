#ifndef	__SERVICE_IPC_INCLUDE_H__
#define	__SERVICE_IPC_INCLUDE_H__

#include "lib_acl.h"
#include "service_struct.h"

#ifdef	__cplusplus
extern "C" {
#endif

void service_ipc_init(ACL_AIO *aio, int nthreads);
void service_ipc_add_service(SERVICE *service,
	module_service_main_fn service_callback);
void service_ipc_add(ACL_SOCKET fd);

#ifdef	__cplusplus
}
#endif

#endif
