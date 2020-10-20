#include "stdafx.h"

#include "fiber/fiber_define.h"
#include "fiber/fiber_hook.h"

#ifndef SYS_UNIX
#include "common/pthread_patch.h"
#endif

#include "common/msg.h"
#include "common/argv.h"
#include "common/htable.h"
#include "common/strops.h"
#include "rfc1035.h"
#include "sane_inet.h"
#include "resolver.h"

typedef struct resolv_conf {
	ARGV *nameservers;
} resolv_conf;

static char *__resolv_file   = NULL;
static resolv_conf *__resolv = NULL;

static char *__hosts_file    = NULL;
static HTABLE *__hosts       = NULL;

static char *__services_file = NULL;
static HTABLE *__services    = NULL;

static void free_service(void *arg)
{
	SERVICE_PORT *service = (SERVICE_PORT*) arg;

	argv_free(service->transports);
	free(service);
}

static void resolver_end(void)
{
	if (__resolv) {
		argv_free(__resolv->nameservers);
		free(__resolv);
		__resolv = NULL;
	}
	if (__resolv_file) {
		free(__resolv_file);
		__resolv_file = NULL;
	}

	if (__hosts) {
		htable_free(__hosts, free);
		__hosts = NULL;
	}
	if (__hosts_file) {
		free(__hosts_file);
		__hosts_file = NULL;
	}

	if (__services) {
		htable_free(__services, free_service);
		__services = NULL;
	}
	if (__services_file) {
		free(__services_file);
		__services_file = NULL;
	}
}

#define SKIP_WHILE(cond, ptr) { while(*ptr && (cond)) ptr++; }
#define IS_SPACE(x) ((x) == ' ' || (x) == '\t' || (x) == '\r' || (x) == '\n')
#define EQ(x, y) !strcasecmp((x), (y))

static void load_reolv_conf(const char *file, resolv_conf *conf)
{
	FILE *fp = fopen(file, "r");
	char buf[1024];

	if (fp == NULL) {
		msg_error("open %s error %s", file, last_serror());
		return;
	}

	while (!feof(fp)) {
		ARGV *tokens;
		char *s = fgets(buf, (int) sizeof(buf), fp);
		if (s == NULL) {
			break;
		}

		SKIP_WHILE(IS_SPACE(*s), s);
		if (*s == '#' || *s == 0) {
			continue;
		}

		tokens = argv_split(s, " \t\r\n");
		if (tokens->argc < 2 || !EQ(tokens->argv[0], "nameserver")
			|| !is_ip(tokens->argv[1])) {
			argv_free(tokens);
			continue;
		}
		argv_add(conf->nameservers, tokens->argv[1], NULL);
		argv_free(tokens);
	}

	fclose(fp);
}

static void host_add(HTABLE *hosts, const char *line)
{
	ARGV *tokens;
	char *key, *ip;
	HOST_LOCAL *host;
	int ip_type;

	SKIP_WHILE(IS_SPACE(*line), line);
	if (*line == '#' || *line == 0) {
		return;
	}

	tokens = argv_split(line, " \t\r\n");
	if (tokens->argc < 2) {
		argv_free(tokens);
		return;
	}
	if (is_ipv4(tokens->argv[0])) {
		ip_type = AF_INET;
#ifdef	AF_INET6
	} else if (is_ipv6(tokens->argv[0])) {
		ip_type = AF_INET6;
#endif
	} else {
		msg_warn("not valid ip format: %s in line=%s",
			 tokens->argv[0], line);
		argv_free(tokens);
		return;
	}

	ip  = tokens->argv[0];
	key = tokens->argv[1];
	lowercase(key);
	host = (struct HOST_LOCAL*) htable_find(hosts, key);
	if (host == NULL) {
		host = (HOST_LOCAL*) calloc(1, sizeof(struct HOST_LOCAL));
		host->ipv4[0] = 0;
		host->ipv6[0] = 0;
		htable_enter(hosts, key, host);
	}
	if (ip_type == AF_INET) {
		SAFE_STRNCPY(host->ipv4, ip, sizeof(host->ipv4));
#ifdef	AF_INET6
	} else if (ip_type == AF_INET6) {
		SAFE_STRNCPY(host->ipv6, ip, sizeof(host->ipv6));
#endif
	}
	argv_free(tokens);
}

static void load_hosts_conf(const char *file, HTABLE *hosts)
{
	FILE *fp = fopen(file, "r");
	char buf[1024];

	if (fp == NULL) {
		msg_error("open %s error %s", file, last_serror());
		return;
	}

	while (!feof(fp)) {
		char *s = fgets(buf, (int) sizeof(buf), fp);
		if (s == NULL) {
			break;
		}

		SKIP_WHILE(IS_SPACE(*s), s);
		if (*s != '#' && *s != 0) {
			host_add(hosts, s);
		}
	}

	fclose(fp);
}

const HOST_LOCAL *find_from_localhost(const char *name)
{
	char buf[256];

	if (__hosts == NULL) {
		return NULL;
	}

	SAFE_STRNCPY(buf, name, sizeof(buf));
	lowercase(buf);
	return (const HOST_LOCAL*) htable_find(__hosts, buf);
}

static void service_add(HTABLE *services, const char *line)
{
	char *name, *port, *transport;
	SERVICE_PORT *service;
	int i, found;
	ARGV *tokens;

	SKIP_WHILE(IS_SPACE(*line), line);
	if (*line == '#' || *line == 0) {
		return;
	}

	tokens = argv_split(line, " \t\r\n");
	if (tokens->argc < 2) {
		argv_free(tokens);
		return;
	}

	port = tokens->argv[1];
	if (*port == 0) {
		argv_free(tokens);
		return;
	}

	if (!(transport = strchr(port, '/')) || !*(++transport)) {
		argv_free(tokens);
		return;
	}

	name = tokens->argv[0];
	lowercase(name);
	service = (SERVICE_PORT*) htable_find(services, name);
	if (service == NULL) {
		service = (SERVICE_PORT*) calloc(1, sizeof(SERVICE_PORT));
		service->transports = argv_alloc(2);
		SAFE_STRNCPY(service->name, tokens->argv[0],
			sizeof(service->name));
		service->port = atoi(port);

		htable_enter(services, name, service);
	}

	found = 0;
	for (i = 0; i < service->transports->argc; i++) {
		if (EQ(service->transports->argv[i], transport)) {
			found = 1;
		}
	}
	if (!found) {
		argv_add(service->transports, transport, NULL);
	}
	argv_free(tokens);
}

static void load_services_conf(const char *file, HTABLE *services)
{
	FILE *fp = fopen(file, "r");
	char buf[1024];

	if (fp == NULL) {
		msg_error("open %s error %s", file, last_serror());
		return;
	}

	while (!feof(fp)) {
		char *s = fgets(buf, (int) sizeof(buf), fp);
		if (s == NULL) {
			break;
		}
		service_add(services, s);
	}

	fclose(fp);
}

unsigned short get_service_port(const char *name)
{
	SERVICE_PORT *service;
	char key[128];

	if (name == NULL || *name == 0) {
		return 0;
	}

	if (alldig(name)) {
		return (unsigned short) atoi(name);
	}

	if (__services == NULL) {
		return 0;
	}

	SAFE_STRNCPY(key, name, sizeof(key));
	lowercase(key);
	service = (SERVICE_PORT*) htable_find(__services, key);
	if (service == NULL) {
		return 0;
	}
	return service->port;
}

static void resolver_init(void)
{
	const char *resolv_default   = "/etc/resolv.conf";
	const char *hosts_default    = "/etc/hosts";
	const char *services_default = "/etc/services";
	const char *resolv_file = __resolv_file ? __resolv_file : resolv_default;
	const char *hosts_file = __hosts_file ? __hosts_file : hosts_default;
	const char *services_file = __services_file ? __services_file : services_default;

	__resolv   = (struct resolv_conf*) malloc(sizeof(resolv_conf));
	__resolv->nameservers = argv_alloc(5);
	__hosts    = htable_create(10);
	__services = htable_create(10000);

	atexit(resolver_end);

	load_reolv_conf(resolv_file, __resolv);
	load_hosts_conf(hosts_file, __hosts);
	load_services_conf(services_file, __services);
}

static pthread_once_t  __once_control = PTHREAD_ONCE_INIT;

void resolver_init_once(void)
{
	pthread_once(&__once_control, resolver_init);
}

static unsigned short __qid = 0;

static unsigned short get_next_qid(void)
{
	return __qid++;
}

static int udp_request(const char *ip, unsigned short port,
	const char *data, size_t dlen, char *buf, size_t size)
{
	int ret;
	struct sockaddr_in addr, from_addr;
	socklen_t len;
	socket_t sock = acl_fiber_socket(AF_INET, SOCK_DGRAM, 0);

	if (sock == INVALID_SOCKET) {
		msg_error("%s(%d): create socket error %s",
			  __FUNCTION__ , __LINE__, last_serror());
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	ret = sendto(sock, data, dlen, 0, (struct sockaddr *) &addr,
		     (socklen_t) sizeof(addr));
	if (ret < 0) {
		msg_error("%s(%d): send error %s",
			__FUNCTION__ , __LINE__, last_serror());
		acl_fiber_close(sock);
		return -1;
	}

	len = (socklen_t) sizeof(from_addr);
	ret = acl_fiber_recvfrom(sock, buf, size, 0,
		(struct sockaddr*) &from_addr, &len);
	acl_fiber_close(sock);
	if (ret <= 0) {
		msg_error("%s(%d): read error %s",
			__FUNCTION__ , __LINE__, last_serror());
		return -1;
	}
	return ret;
}

static struct addrinfo * rfc1035_to_addrinfo(const RFC1035_MESSAGE *message,
	int count, unsigned short service_port)
{
	int i;
	struct addrinfo *res = NULL, *ent;

	for (i = 0; i < count; i++) {
		if (message->answer[i].type == RFC1035_TYPE_A) {
			struct sockaddr_in *in = (struct sockaddr_in*)
				calloc(1, sizeof(struct sockaddr_in));
			ent = (struct addrinfo*) calloc(1, sizeof(*ent));

			ent->ai_family = AF_INET;
			ent->ai_addrlen = sizeof(struct sockaddr_in);
			ent->ai_addr = (struct sockaddr*) in;

			memcpy(&in->sin_addr, message->answer[i].rdata, 4);
			in->sin_family = AF_INET;
			in->sin_port = htons(service_port);
			in->sin_len  = sizeof(struct sockaddr_in);
#ifdef	AF_INET6
		} else if (message->answer[i].type == RFC1035_TYPE_AAAA) {
			struct sockaddr_in6 *in = (struct sockaddr_in6*)
				calloc(1, sizeof(struct sockaddr_in6));
			ent = (struct addrinfo*) calloc(1, sizeof(ent));

			ent->ai_family = AF_INET6;
			ent->ai_addrlen = sizeof(struct sockaddr_in6);
			ent->ai_addr = (struct sockaddr*) in;

			memcpy(&in->sin6_addr, message->answer[i].rdata, 16);
			in->sin6_family = AF_INET6;
			in->sin6_port = htons(service_port);
			in->sin6_len = sizeof(struct sockaddr_in6);
#endif
		} else {
			continue;
		}
		ent->ai_flags = 0;
		ent->ai_socktype = SOCK_DGRAM;
		ent->ai_protocol = 0;
		ent->ai_next = res;
		res = ent;
	}

	return res;
}

static struct addrinfo *ns_lookup(const char *ip, unsigned short port,
	const char *data, size_t dlen, unsigned short service_port)
{
	RFC1035_MESSAGE *message;
	struct addrinfo *res;
	char buf[1000];
	int ret;

	ret = udp_request(ip, port, data, dlen, buf, sizeof(buf));
	if (ret == -1) {
		return NULL;
	}
	ret = rfc1035_message_unpack(buf, ret, &message);
	if (ret <= 0) {
		if (message) {
			rfc1035_message_destroy(message);
		}
		return NULL;
	}

	res = rfc1035_to_addrinfo(message, ret, service_port);
	rfc1035_message_destroy(message);
	return res;
}

struct addrinfo *resolver_getaddrinfo(const char *name, const char *service,
	const struct addrinfo* hints)
{
	RFC1035_QUERY query;
	char buf[1000];
	size_t size;
	int type, i;
	unsigned short service_port;

	if (__resolv == NULL || __resolv->nameservers->argc <= 0) {
		return NULL;
	}

	if (hints) {
		switch (hints->ai_family) {
		case AF_INET6:
			type = RFC1035_TYPE_AAAA;
			break;
		case AF_INET:
		default:
			type = RFC1035_TYPE_A;
			break;
		}
	} else {
		type = RFC1035_TYPE_A;
	}

	size = rfc1035_build_query(name, buf, sizeof(buf), get_next_qid(), type,
			RFC1035_CLASS_IN, &query);
	if (size == 0) {
		msg_error("%s(%d): rfc1035_build_query4a error, name=%s",
			  __FUNCTION__ , __LINE__, name);
		return NULL;
	}

	service_port = get_service_port(service);

	for (i = 0; i < __resolv->nameservers->argc; i++) {
		const char *ip = __resolv->nameservers->argv[i];
		struct addrinfo *res = ns_lookup(ip, 53, buf, size, service_port);
		if (res != NULL) {
			return res;
		}
	}

	return NULL;
}

void resolver_freeaddrinfo(struct addrinfo *res)
{
	while (res) {
		struct addrinfo *ent = res;
		res = res->ai_next;
		free(ent->ai_addr);
		free(ent);
	}
}