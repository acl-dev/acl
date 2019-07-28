#include "stdafx.h"
#include "service_main.h"

void create_key(char *key, size_t size, char type, unsigned short id)
{
	const char *myname = "create_key";

	if (type == SERVICE_CTX_TCP_REQUEST)
		snprintf(key, size, "TCP:REQUEST:%d", id);
	else if (type == SERVICE_CTX_TCP_RESPOND)
		snprintf(key, size, "TCP:RESPOND:%d", id);
	else if (type == SERVICE_CTX_UDP_REQUEST)
		snprintf(key, size, "UDP:REQUEST:%d", id);
	else if (type == SERVICE_CTX_UDP_RESPOND)
		snprintf(key, size, "UDP:RESPOND:%d", id);
	else
		acl_msg_fatal("%s(%d): type(%d) invalid",
			myname, __LINE__, type);
}

SERVICE_CTX *service_ctx_new(SERVICE *service, ACL_ASTREAM *stream,
	char type, unsigned short id)
{
	const char *myname = "service_ctx_new";
	SERVICE_CTX *ctx = (SERVICE_CTX*) acl_mycalloc(1, sizeof(SERVICE_CTX));

	ctx->service = service;
	ctx->stream = stream;
	ctx->type = type;
	ctx->id = id;

	create_key(ctx->key, sizeof(ctx->key), type, id);

	if (acl_htable_enter(service->table, ctx->key, ctx) == NULL)
		acl_msg_fatal("%s(%d): enter to table error, key(%s)",
			myname, __LINE__, ctx->key);
	return (ctx);
}

void service_ctx_free(SERVICE_CTX *ctx)
{
	acl_htable_delete(ctx->service->table, ctx->key, NULL);
	acl_myfree(ctx);
}

SERVICE_CTX *service_ctx_find(SERVICE *service, char type, unsigned int id)
{
	SERVICE_CTX *ctx;
	char  key[KEY_LEN];

	create_key(key, sizeof(key), type, id);
	ctx = (SERVICE_CTX*) acl_htable_find(service->table, key);
	return (ctx);
}

static int accept_callback(ACL_ASTREAM *client, void *context)
{
	SERVICE *service = (SERVICE*) context;

	service_tcp_main(client, service);
	return (0);
}

void service_start(SERVICE *service)
{
	while (1)
		acl_aio_loop(service->aio);
}

SERVICE *service_create(const char *local_ip, short local_port,
	const char *dns_ip, short dns_port)
{
	const char *myname = "service_create";
	SERVICE *service;
	ACL_VSTREAM *sstream;
	char addr[64];

	// 创建提供 TCP 方式查询时的监听流
	snprintf(addr, sizeof(addr), "%s:%d", local_ip, local_port);
	sstream = acl_vstream_listen_ex(addr, 128, ACL_NON_BLOCKING, 1024, 10);
	if (sstream == NULL) {
		acl_msg_error("%s(%d): can't listen on addr(%s)",
			myname, __LINE__, addr);
		return (NULL);
	}

	service = (SERVICE*) acl_mycalloc(1, sizeof(SERVICE));
	ACL_SAFE_STRNCPY(service->listen_addr,
		addr, sizeof(service->listen_addr));
	ACL_SAFE_STRNCPY(service->dns_ip, dns_ip, sizeof(service->dns_ip));
	service->dns_port = dns_port;
	snprintf(service->dns_addr, sizeof(service->dns_addr),
		"%s:%d", dns_ip, dns_port);
	service->conn_timeout = 10;
	service->rw_timeout = 10;

	service->table = acl_htable_create(100, 0);
	service->aio = acl_aio_create(ACL_EVENT_SELECT);
	service->sstream = acl_aio_open(service->aio, sstream);
	acl_aio_ctl(service->sstream, ACL_AIO_CTL_ACCEPT_FN, accept_callback,
		ACL_AIO_CTL_CTX, service, ACL_AIO_CTL_END);

	acl_aio_accept(service->sstream);
	service_udp_init(service, local_ip, local_port, dns_ip, dns_port);
	return (service);
}

void service_free(SERVICE *service)
{
	// XXX: aio have no free function
	acl_myfree(service);
}
