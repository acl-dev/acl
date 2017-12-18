#include "stdafx.h"
#include "common.h"
#include "netdb.h"

static void free_dns_db(void *ctx)
{
	free(ctx);
}

void netdb_free(DNS_DB *h_dns_db)
{
	if (h_dns_db == NULL)
		return;
	if (h_dns_db->h_db)
		array_destroy(h_dns_db->h_db, free_dns_db);
	free(h_dns_db);
}

static const HOST_INFO *netdb_iter_head(ITER *iter, struct DNS_DB *dns_db)
{
	return (const HOST_INFO*) dns_db->h_db->iter_head(iter, dns_db->h_db);
}

static const HOST_INFO *netdb_iter_next(ITER *iter, struct DNS_DB *dns_db)
{
	return (const HOST_INFO*) dns_db->h_db->iter_next(iter, dns_db->h_db);
}

static const HOST_INFO *netdb_iter_tail(ITER *iter, struct DNS_DB *dns_db)
{
	return (const HOST_INFO*) dns_db->h_db->iter_tail(iter, dns_db->h_db);
}

static const HOST_INFO *netdb_iter_prev(ITER *iter, struct DNS_DB *dns_db)
{
	return (const HOST_INFO*) dns_db->h_db->iter_prev(iter, dns_db->h_db);
}

static const HOST_INFO *netdb_iter_info(ITER *iter,
	struct DNS_DB *dns_db unused)
{
	return iter->ptr ? (HOST_INFO*) iter->ptr : NULL;
}

DNS_DB *netdb_new(const char *domain)
{
	const char *name = "netdb_new";
	DNS_DB *dns_db;
	char  buf[256];

	dns_db = calloc(1, sizeof(DNS_DB));
	dns_db->h_db = array_create(5);
	if (dns_db->h_db == NULL) {
		msg_error("%s, %s(%d): create array error(%s)",
			__FILE__, name, __LINE__,
			last_strerror(buf, sizeof(buf)));
		free(dns_db);
		return NULL;
	}

	snprintf(dns_db->name, sizeof(dns_db->name), "%s", domain);
	lowercase(dns_db->name);

	dns_db->iter_head = netdb_iter_head;
	dns_db->iter_next = netdb_iter_next;
	dns_db->iter_tail = netdb_iter_tail;
	dns_db->iter_prev = netdb_iter_prev;
	dns_db->iter_info = netdb_iter_info;

	return dns_db;
}
