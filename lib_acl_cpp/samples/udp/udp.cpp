/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Thu 30 May 2017 05:25:43 PM CST
 */
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

udp_pkt::udp_pkt(size_t size /* = 1460 */)
	: size_(size)
{
	acl_assert(size >= 1460);
	iov_.iov_base = acl_mymalloc(size);
	iov_.iov_len  = size;
}

udp_pkt::~udp_pkt(void)
{
	acl_myfree(iov_.iov_base);
}

bool udp_pkt::set_data(const void* data, size_t len)
{
	if (data == NULL)
	{
		logger_error("data null");
		return false;
	}
	if (len == 0 || len > size_)
	{
		logger_error("invalid len=%lu", len);
		return false;
	}
	memcpy(iov_.iov_base, data, len);
	iov_.iov_len  = len;
	return true;
}

bool udp_pkt::set_peer(const char* addr)
{
	struct addrinfo *peer_res0 = host_addrinfo(addr);

	if (peer_res0 == NULL)
		return false;

	memcpy(&addr_, peer_res0->ai_addr, peer_res0->ai_addrlen);
	addr_len_ = peer_res0->ai_addrlen;
	freeaddrinfo(peer_res0);
	return true;
}

int udp_pkt::get_port(void) const
{
	if (addr_.sa.sa.sa_family == AF_INET)
	{
		const struct sockaddr_in *in = &addr_.sa.in;
		return ntohs(in->sin_port);
	}
#ifdef AF_INET6
	else if (addr_.sa.sa.sa_family == AF_INET6)
	{
		const struct sockaddr_in6 *in = &addr_.sa.in6;
		return ntohl(in->sin6_port);
	}
#endif
	else
	{
		logger_error("unkown sa_family=%d", addr_.sa.sa.sa_family);
		return -1;
	}
}

const char *udp_pkt::get_ip(char* buf, size_t size) const
{
	if (addr_.sa.sa.sa_family == AF_INET)
	{
		const struct sockaddr_in *in = &addr_.sa.in;
		return inet_ntop(in->sin_family, &in->sin_addr, buf, size);
	}
#ifdef AF_INET6
	else if (addr_.sa.sa.sa_family == AF_INET6)
	{
		const struct sockaddr_in6 *in = &addr_.sa.in6;
		return inet_ntop(in->sin6_family, &in->sin6_addr, buf, size);
	}
#endif
	else
	{
		logger_error("unkown sa_family=%d", addr_.sa.sa.sa_family);
		return NULL;
	}
}

udp_pkts::udp_pkts(size_t n)
{
	for (size_t i = 0; i < n; i++)
	{
		udp_pkt* pkt = new udp_pkt;
		pkts_.push_back(pkt);
	}
}

udp_pkts::~udp_pkts(void)
{
	for (std::vector<udp_pkt*>::iterator it = pkts_.begin();
		it != pkts_.end(); ++it)
	{
		delete *it;
	}
}

udp_pkt* udp_pkts::operator[](size_t i)
{
	if (i >= pkts_.size())
		return NULL;
	return pkts_[i];
}

udp_sock::udp_sock(void)
	: fd_(ACL_SOCKET_INVALID)
	, msgvec_(NULL)
	, vlen_(0)
{
}

udp_sock::~udp_sock(void)
{
	acl_myfree(msgvec_);
	if (fd_ != ACL_SOCKET_INVALID)
		acl_socket_close(fd_);
}

bool udp_sock::bind(const char* addr)
{
	struct addrinfo *res0, *res;

	res0 = host_addrinfo(addr);
	if (res0 == NULL)
		return false;

	fd_ = bind_addr(res0, &res);
	if (fd_ == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket");
		freeaddrinfo(res0);
		return false;
	}

	memcpy(&sa_local_, res->ai_addr, res->ai_addrlen);
	sa_local_len_ = res->ai_addrlen;

	freeaddrinfo(res0);
	return true;
}

bool udp_sock::server_open(const char *local)
{
	return bind(local);
}

bool udp_sock::client_open(const char *local, const char *peer)
{
	struct addrinfo *peer_res0 = host_addrinfo(peer);

	if (peer_res0 == NULL)
		return NULL;

	if (bind(local) == false)
	{
		freeaddrinfo(peer_res0);
		return false;
	}

	memcpy(&sa_peer_, peer_res0->ai_addr, peer_res0->ai_addrlen);
	sa_peer_len_ = peer_res0->ai_addrlen;
	freeaddrinfo(peer_res0);
	return true;
}

ssize_t udp_sock::send(const void *data, size_t len)
{
	return sendto(fd_, data, len, 0,
		(struct sockaddr *) &sa_peer_, sa_peer_len_);
}

ssize_t udp_sock::recv(void *buf, size_t size)
{
	sa_peer_len_ = sizeof(sa_peer_);
	return recvfrom(fd_, buf, size, 0,
		(struct sockaddr *) &sa_peer_, &sa_peer_len_);
}

int udp_sock::send(std::vector<udp_pkt*>& pkts, size_t max)
{
	size_t pkts_cnt = pkts.size();
	if (pkts_cnt >= max)
		pkts_cnt = max;

	if (msgvec_ == NULL)
	{
		vlen_   = pkts_cnt;
		msgvec_ = (struct mmsghdr *)
			acl_mycalloc(pkts_cnt, sizeof(struct mmsghdr));
	}
	else if (vlen_ < pkts_cnt)
	{
		acl_myfree(msgvec_);
		vlen_   = pkts_cnt;
		msgvec_ = (struct mmsghdr *)
			acl_mycalloc(pkts_cnt, sizeof(struct mmsghdr));
	}

	unsigned int flags = 0;
	memset(msgvec_, 0, sizeof(struct mmsghdr) * pkts_cnt);

#ifndef __linux3__
	int   n = 0;
#endif

	for (size_t i = 0; i < pkts_cnt; i++)
	{
		msgvec_[i].msg_hdr.msg_iov     = &pkts[i]->iov_;
		msgvec_[i].msg_hdr.msg_iovlen  = 1;
		msgvec_[i].msg_hdr.msg_name    = &pkts[i]->addr_;
		msgvec_[i].msg_hdr.msg_namelen = sizeof(pkts[i]->addr_);
		msgvec_[i].msg_len             = 0;
#ifndef __linux3__
		if (sendmsg(fd_, &msgvec_[i].msg_hdr, flags) < 0)
			return -1;
		n++;
#endif
	}

#ifdef __linux3__
	return sendmmsg(fd_, msgvec_, pkts_cnt, flags);
#else
	return n;
#endif
}

int udp_sock::recv(std::vector<udp_pkt*>& pkts)
{
	unsigned int flags = MSG_WAITFORONE /* | MSG_DONTWAIT */;
	size_t pkts_cnt = pkts.size();

	if (msgvec_ == NULL)
	{
		vlen_   = pkts_cnt;
		msgvec_ = (struct mmsghdr *)
			acl_mycalloc(pkts_cnt, sizeof(struct mmsghdr));
	}
	else if (vlen_ < pkts_cnt)
	{
		acl_myfree(msgvec_);
		vlen_   = pkts_cnt;
		msgvec_ = (struct mmsghdr *)
			acl_mycalloc(pkts_cnt, sizeof(struct mmsghdr));
	}

	memset(msgvec_, 0, sizeof(struct mmsghdr) * pkts_cnt);

	int i = 0;
	for (std::vector<udp_pkt*>::iterator it = pkts.begin();
		it != pkts.end(); ++it)
	{
		(*it)->iov_.iov_len            = (*it)->size_;
		msgvec_[i].msg_hdr.msg_iov     = &(*it)->iov_;
		msgvec_[i].msg_hdr.msg_iovlen  = 1;
		msgvec_[i].msg_hdr.msg_name    = &(*it)->addr_;
		msgvec_[i].msg_hdr.msg_namelen = sizeof((*it)->addr_);
		msgvec_[i].msg_len             = 0;
		++i;
	}

	int ret = recvmmsg(fd_, msgvec_, pkts_cnt, flags, NULL);
	if (ret <= 0)
		return ret;

	for (i = 0; i < ret; i++)
		msgvec_[i].msg_hdr.msg_iov->iov_len = msgvec_[i].msg_len;

	return ret;
}
