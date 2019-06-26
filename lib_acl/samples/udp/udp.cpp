#include "stdafx.h"
#include "udp.h"

static bool host_port(char *buf, char **host, char **port)
{
	const char *ptr = acl_host_port(buf, host, "", port, (char*) NULL);

	if (ptr != NULL) {
		acl_msg_error("%s(%d): invalid addr %s, %s",
			__FILE__, __LINE__, buf, ptr);
		return false;
	}

	if (*port == NULL || atoi(*port) < 0) {
		acl_msg_error("%s(%d): invalid port: %s, addr: %s",
			__FILE__, __LINE__, *port ? *port : "null", buf);
		return false;
	}

	if (*host && **host == 0)
		*host = 0;
	if (*host == NULL)
		*host = "0";

	return true;
}

static struct addrinfo *host_addrinfo(const char *addr)
{
	int    err;
	struct addrinfo hints, *res0;
	char  *buf = acl_mystrdup(addr), *host = NULL, *port = NULL;

	if (host_port(buf, &host, &port) == false) {
		acl_myfree(buf);
		return NULL;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
#ifdef  ACL_MACOSX
	hints.ai_flags    = AI_DEFAULT;
#elif   defined(ACL_ANDROID)
	hints.ai_flags    = AI_ADDRCONFIG;
#elif defined(ACL_WINDOWS)
	hints.ai_protocol = IPPROTO_UDP;
# if _MSC_VER >= 1500
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
# endif
#else
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
#endif
	if ((err = getaddrinfo(host, port, &hints, &res0))) {
		acl_msg_error("%s(%d): getaddrinfo error %s, peer=%s",
			__FILE__, __LINE__, gai_strerror(err), host);
		acl_myfree(buf);
		return NULL;
	}

	acl_myfree(buf);
	return res0;
}

static int bind_addr(struct addrinfo *res0, struct addrinfo **res)
{
	struct addrinfo *it;
	int   on, fd;

	for (it = res0; it != NULL ; it = it->ai_next) {
		fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (fd == ACL_SOCKET_INVALID) {
			acl_msg_error("%s(%d): create socket %s",
				__FILE__, __LINE__, acl_last_serror());
			return ACL_SOCKET_INVALID;
		}

		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			(const void *) &on, sizeof(on)) < 0)
		{
			acl_msg_warn("%s(%d): setsockopt(SO_REUSEADDR): %s",
				__FILE__, __LINE__, acl_last_serror());
		}

#if defined(SO_REUSEPORT)
		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
			(const void *) &on, sizeof(on)) < 0)
		{
			acl_msg_warn("%s(%d): setsocket(SO_REUSEPORT): %s",
				__FILE__, __LINE__, acl_last_serror());
		}
#endif

#ifdef ACL_WINDOWS
		if (bind(fd, it->ai_addr, (int) it->ai_addrlen) == 0)
#else
		if (bind(fd, it->ai_addr, it->ai_addrlen) == 0)
#endif
		{
			*res = it;
			return fd;
		}

		acl_msg_error("%s(%d): bind error %s",
			__FILE__, __LINE__, acl_last_serror());
		acl_socket_close(fd);
	}

	return ACL_SOCKET_INVALID;
}

static SOCK_UDP *sock_open(const char *addr)
{
	SOCK_UDP *sock;
	struct addrinfo *res0, *res;
	int    fd;

	res0 = host_addrinfo(addr);
	if (res0 == NULL)
		return NULL;

	fd = bind_addr(res0, &res);
	if (fd == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d): invalid socket", __FILE__, __LINE__);
		freeaddrinfo(res0);
		return NULL;
	}

	sock = (SOCK_UDP *) acl_mycalloc(1, sizeof(SOCK_UDP));
	sock->fd = fd;
	memcpy(&sock->sa_local, res->ai_addr, res->ai_addrlen);
	sock->sa_local_len = res->ai_addrlen;

	freeaddrinfo(res0);
	return sock;
}

SOCK_UDP *udp_client_open(const char *local, const char *peer)
{
	struct addrinfo *peer_res0 = host_addrinfo(peer);
	SOCK_UDP *sock;

	if (peer_res0 == NULL)
		return NULL;

	sock = sock_open(local);
	if (sock == NULL) {
		freeaddrinfo(peer_res0);
		return NULL;
	}

	memcpy(&sock->sa_peer, peer_res0->ai_addr, peer_res0->ai_addrlen);
	sock->sa_peer_len = peer_res0->ai_addrlen;
	freeaddrinfo(peer_res0);
	return sock;
}

SOCK_UDP *udp_server_open(const char *local)
{
	return sock_open(local);
}

void udp_close(SOCK_UDP *sock)
{
	acl_socket_close(sock->fd);
	acl_myfree(sock);
}

int udp_read(SOCK_UDP *sock, void *buf, size_t size)
{
	ssize_t ret;

	sock->sa_peer_len = sizeof(sock->sa_peer);
	ret = recvfrom(sock->fd, buf, size, 0,
		(struct sockaddr *) &sock->sa_peer, &sock->sa_peer_len);
	return ret;
}

int udp_send(SOCK_UDP *sock, const void *data, size_t len)
{
	ssize_t ret = sendto(sock->fd, data, len, 0,
		(struct sockaddr *) &sock->sa_peer, sock->sa_peer_len);
	return ret;
}

void udp_pkt_set_buf(SOCK_PKT *pkt, char *buf, size_t len)
{
	pkt->iov.iov_base = buf;
	pkt->iov.iov_len  = len;
	pkt->addr_len     = (socklen_t) sizeof(SOCK_ADDR);
}

int udp_pkt_set_peer(SOCK_PKT *pkt, const char *addr)
{
	struct addrinfo *peer_res0 = host_addrinfo(addr);

	if (peer_res0 == NULL)
		return -1;
	memcpy(&pkt->addr, peer_res0->ai_addr, peer_res0->ai_addrlen);
	pkt->addr_len = peer_res0->ai_addrlen;
	freeaddrinfo(peer_res0);
	return 0;
}

int udp_mread(SOCK_UDP *sock, SOCK_PKT pkts[], size_t pkts_cnt)
{
	unsigned int flags = MSG_WAITFORONE /* | MSG_DONTWAIT */;
	size_t i;

	if (sock->msgvec == NULL) {
		sock->vlen   = pkts_cnt;
		sock->msgvec = (struct mmsghdr *)
			acl_mycalloc(pkts_cnt, sizeof(struct mmsghdr));
	} else if (sock->vlen < pkts_cnt) {
		acl_myfree(sock->msgvec);
		sock->vlen   = pkts_cnt;
		sock->msgvec = (struct mmsghdr *)
			acl_mycalloc(pkts_cnt, sizeof(struct mmsghdr));
	}

	sock->pkts     = pkts;
	sock->pkts_cnt = pkts_cnt;

	memset(sock->msgvec, 0, sizeof(struct mmsghdr) * pkts_cnt);

	for (i = 0; i < pkts_cnt; i++) {
		sock->msgvec[i].msg_hdr.msg_iov     = &pkts[i].iov;
		sock->msgvec[i].msg_hdr.msg_iovlen  = 1;
		sock->msgvec[i].msg_hdr.msg_name    = &pkts[i].addr;
		sock->msgvec[i].msg_hdr.msg_namelen = sizeof(pkts[i].addr);
		sock->msgvec[i].msg_len             = 0;
	}

	return recvmmsg(sock->fd, sock->msgvec, pkts_cnt, flags, NULL);
}

int udp_msend(SOCK_UDP *sock, SOCK_PKT pkts[], size_t pkts_cnt)
{
	size_t i;
	unsigned int flags = 0;
#ifndef __linux3__
	int   n = 0;
#endif

	if (sock->msgvec == NULL) {
		sock->vlen   = pkts_cnt;
		sock->msgvec = (struct mmsghdr *)
			acl_mycalloc(pkts_cnt, sizeof(struct mmsghdr));
	} else if (sock->vlen < pkts_cnt) {
		acl_myfree(sock->msgvec);
		sock->vlen   = pkts_cnt;
		sock->msgvec = (struct mmsghdr *)
			acl_mycalloc(pkts_cnt, sizeof(struct mmsghdr));
	}

	sock->pkts     = pkts;
	sock->pkts_cnt = pkts_cnt;

	memset(sock->msgvec, 0, sizeof(struct mmsghdr) * pkts_cnt);

	for (i = 0; i < pkts_cnt; i++) {
		sock->msgvec[i].msg_hdr.msg_iov     = &pkts[i].iov;
		sock->msgvec[i].msg_hdr.msg_iovlen  = 1;
		sock->msgvec[i].msg_hdr.msg_name    = &pkts[i].addr;
		sock->msgvec[i].msg_hdr.msg_namelen = sizeof(pkts[i].addr);
		sock->msgvec[i].msg_len             = 0;
#ifndef __linux3__
		if (sendmsg(sock->fd, &sock->msgvec[i].msg_hdr, flags) < 0)
			return -1;
		n++;
#endif
	}

#ifdef __linux3__
	return sendmmsg(sock->fd, sock->msgvec, pkts_cnt, flags);
#else
	return n;
#endif
}

int pkt_port(SOCK_PKT *pkt)
{
	if (pkt->addr.sa.sa.sa_family == AF_INET) {
		struct sockaddr_in *in = &pkt->addr.sa.in;
		return ntohs(in->sin_port);
	}
#ifdef AF_INET6
	else if (pkt->addr.sa.sa.sa_family == AF_INET6) {
		struct sockaddr_in6 *in = &pkt->addr.sa.in6;
		return ntohl(in->sin6_port);
	}
#endif
	else {
		acl_msg_error("%s(%d): unkown sa_family=%d",
			__FUNCTION__, __LINE__, pkt->addr.sa.sa.sa_family);
		return -1;
	}
}

int udp_port(SOCK_UDP *sock, size_t i)
{
	if (sock->msgvec == NULL) {
		acl_msg_error("%s(%d): msgvec NULL", __FUNCTION__, __LINE__);
		return -1;
	}
	if (sock->pkts == NULL) {
		acl_msg_error("%s(%d): pkts NULL", __FUNCTION__, __LINE__);
		return -1;
	}
	if (sock->pkts_cnt == 0) {
		acl_msg_error("%s(%d): pkts_cnt 0", __FUNCTION__, __LINE__);
		return -1;
	}
	if (i >= sock->pkts_cnt) {
		acl_msg_error("%s(%d): invalid i=%d >= %d",
			__FUNCTION__, __LINE__, (int) i, (int) sock->pkts_cnt);
		return -1;
	}

	return pkt_port(&sock->pkts[i]);
}

const char *pkt_ip(SOCK_PKT *pkt, char *buf, size_t size)
{
	if (pkt->addr.sa.sa.sa_family == AF_INET) {
		struct sockaddr_in *in = &pkt->addr.sa.in;
		return inet_ntop(in->sin_family, &in->sin_addr, buf, size);
	}
#ifdef AF_INET6
	else if (pkt->addr.sa.sa.sa_family == AF_INET6) {
		struct sockaddr_in6 *in = &pkt->addr.sa.in6;
		return inet_ntop(in->sin6_family, &in->sin6_addr, buf, size);
	}
#endif
	else {
		acl_msg_error("%s(%d): unkown sa_family=%d",
			__FUNCTION__, __LINE__, pkt->addr.sa.sa.sa_family);
		return NULL;
	}
}

const char *udp_ip(SOCK_UDP *sock, size_t i, char *buf, size_t size)
{
	if (sock->msgvec == NULL) {
		acl_msg_error("%s(%d): msgvec NULL", __FUNCTION__, __LINE__);
		return NULL;
	}
	if (sock->pkts == NULL) {
		acl_msg_error("%s(%d): pkts NULL", __FUNCTION__, __LINE__);
		return NULL;
	}
	if (sock->pkts_cnt == 0) {
		acl_msg_error("%s(%d): pkts_cnt 0", __FUNCTION__, __LINE__);
		return NULL;
	}
	if (i >= sock->pkts_cnt) {
		acl_msg_error("%s(%d): invalid i=%d >= %d",
			__FUNCTION__, __LINE__, (int) i, (int) sock->pkts_cnt);
		return NULL;
	}

	return pkt_ip(&sock->pkts[i], buf, size);
}

