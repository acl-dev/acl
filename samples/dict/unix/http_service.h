#ifndef	__HTTP_SERVICE_INCLUDE_H__
#define	__HTTP_SERVICE_INCLUDE_H__

#include "lib_acl.h"

/* in http_service.c */

void http_service_init(void *init_ctx);
void http_service_exit(void *exit_ctx);
int http_service_main(ACL_VSTREAM *client, void *run_ctx);

/* in http_error.c */

void http_error_reply(ACL_VSTREAM *client, int status, const char *msg);

#endif
