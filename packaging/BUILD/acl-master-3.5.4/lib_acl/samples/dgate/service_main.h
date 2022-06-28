#ifndef __SERVICE_MAIN_INCLUDE_H__
#define __SERVICE_MAIN_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SERVICE SERVICE;

#define MAX_DOMAIN_LEN	256

struct SERVICE
{
	unsigned short curr_id;
	ACL_HTABLE *table;
	ACL_AIO *aio;
	ACL_ASTREAM *sstream;
	char  listen_addr[256];
	char  dns_addr[256];
	char  dns_ip[32];
	short dns_port;
	char  type;
#define SERVICE_TYPE_UDP	0
#define SERVICE_TYPE_TCP	1

	int   conn_timeout;
	int   rw_timeout;
};

#define KEY_LEN	64
#define MAX_BUF	4096

typedef struct SERVICE_CTX
{
	ACL_ASTREAM *stream;
	SERVICE *service;
	char  domain[256];
	char  domain_root[256];
	char  request_buf[MAX_BUF];
	int   request_len;
	char  respond_buf[MAX_BUF];
	int   respond_len;
	unsigned short id;
	unsigned short id_original;
	unsigned short qtype;
	char  type;
#define SERVICE_CTX_TCP_REQUEST	0
#define SERVICE_CTX_TCP_RESPOND	1
#define SERVICE_CTX_UDP_REQUEST	2
#define SERVICE_CTX_UDP_RESPOND	3
	char  key[KEY_LEN];

	struct sockaddr_in client_addr;
	int   client_addr_len;
} SERVICE_CTX;

/* in service_main.cpp */
void create_key(char *key, size_t size, char type, unsigned short id);
SERVICE_CTX *service_ctx_new(SERVICE *service, ACL_ASTREAM *stream,
	char type, unsigned short id);
void service_ctx_free(SERVICE_CTX *ctx);
SERVICE_CTX *service_ctx_find(SERVICE *service, char type, unsigned int id);
void service_start(SERVICE *service);
SERVICE *service_create(const char *local_ip, short local_port,
	const char *dns_ip, short dns_port);
void service_free(SERVICE *service);

/* in service_tcp.cpp */
void service_tcp_main(ACL_ASTREAM *client, SERVICE *service);

/* in service_udp.cpp */
void service_udp_init(SERVICE *service, const char *local_ip, int local_port,
	const char *remote_ip, int remote_port);

#ifdef __cplusplus
}
#endif

#endif

