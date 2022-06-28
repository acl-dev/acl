#ifndef	__HTTP_SERVICE_INCLUDE_H__
#define	__HTTP_SERVICE_INCLUDE_H__

#include "lib_acl.h"
#include "lib_protocol.h"
#include "dict_pool.h"

#if 0
#define	TRACE() acl_msg_info(">>>%s: %d", __FUNCTION__, __LINE__)
#else
#define	TRACE()
#endif

typedef struct HTTP_CLIENT {
	ACL_ASTREAM *stream;
	HTTP_HDR_REQ *hdr_req;                  /* HTTP协议请求头指针 */
	HTTP_REQ *http_req;
	ACL_VSTRING *sbuf;
	ACL_VSTRING *key;
	DICT_POOL *dict_pool;
} HTTP_CLIENT;

/* http_client.c */

HTTP_CLIENT *http_client_new(ACL_ASTREAM *stream);
void http_client_free(HTTP_CLIENT *client);
void http_client_reset(HTTP_CLIENT *client);

/* http_service.c */
void http_service_init(void *init_ctx);
void http_service_exit(void *exit_ctx);
void http_service_main(ACL_ASTREAM *stream, void *ctx);

/* in http_error.c */
void http_error_reply(HTTP_CLIENT *http_client, int status, const char *msg);
	
#endif
