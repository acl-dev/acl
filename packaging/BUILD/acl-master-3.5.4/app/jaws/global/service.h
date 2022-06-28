#ifndef	__SERVICE_INCLUDE_H__
#define	__SERVICE_INCLUDE_H__

#include "service_struct.h"

#ifdef	__cplusplus
extern "C"
#endif

#define	TRACE	do {  \
	acl_msg_info("%s(%d), %s", __FILE__, __LINE__, __FUNCTION__);  \
} while (0)

/* in service.c */
void service_free(SERVICE *service) ;
SERVICE *service_alloc(const char *service_name, size_t size);
void service_set_dns(SERVICE *service, ACL_AIO *aio,
	const char *dns_list, int dns_lookup_timeout,
	int dns_cache_limit, const char *hosts_list);
void service_set_gctimer(ACL_AIO *aio, int timer);

/* in service_load.c */
void service_load(ACL_FIFO *service_modules, const char *dlname);
void service_load_all(ACL_FIFO *service_modules, const char *dlnames);
void service_unload_all(void);

/* in forward.c */
void forward_complete(CLIENT_ENTRY *entry);
void forward_start(CLIENT_ENTRY *entry);

/* in client_entry.c */
void client_entry_free(CLIENT_ENTRY *entry);
CLIENT_ENTRY *client_entry_new(SERVICE *service, size_t size, ACL_ASTREAM *client);
void client_entry_set_server(CLIENT_ENTRY *entry, ACL_ASTREAM *server);
int client_entry_detach(CLIENT_ENTRY *entry, ACL_VSTREAM *stream);
int client_entry_detach3(CLIENT_ENTRY *entry, ACL_VSTREAM *stream, int auto_free);

#ifdef	__cplusplus
}
#endif

#endif
