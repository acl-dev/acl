#include "stdafx.h"
#include "sock_addr.h"
#include "udp_pkt.h"
#include "udp_sock.h"

static int bind_addr(struct addrinfo *res0, struct addrinfo **res)
{
	struct addrinfo *it;
	int   on, fd;

	for (it = res0; it != NULL ; it = it->ai_next) {
		fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (fd == ACL_SOCKET_INVALID) {
			logger_error("reate socket %s", acl::last_serror());
			return ACL_SOCKET_INVALID;
		}

		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			(const void *) &on, sizeof(on)) < 0)
		{
			logger_warn("setsockopt(SO_REUSEADDR): %s",
				acl::last_serror());
		}

#if defined(SO_REUSEPORT)
		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
			(const void *) &on, sizeof(on)) < 0)
		{
			logger_warn("etsocket(SO_REUSEPORT): %s",
				acl::last_serror());
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

		logger_error("ind error %s", acl::last_serror());
		acl_socket_close(fd);
	}

	return ACL_SOCKET_INVALID;
}

//////////////////////////////////////////////////////////////////////////////

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

	int fd = bind_addr(res0, &res);
	if (fd == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket");
		freeaddrinfo(res0);
		return false;
	}

	memcpy(&sa_local_, res->ai_addr, res->ai_addrlen);
	sa_local_len_ = res->ai_addrlen;

	freeaddrinfo(res0);
	return open(fd);
}

bool udp_sock::open(int fd)
{
	if (fd <= 0)
		return false;
	fd_ = fd;
	return true;
}

bool udp_sock::server_open(const char *local)
{
	return bind(local);
}

bool udp_sock::client_open(const char *local, const char *peer)
{
	if (local == NULL || *local == 0)
	{
		logger_error("local null");
		return NULL;
	}
	if (peer == NULL || *peer == 0)
	{
		logger_error("peer null");
		return false;
	}

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

int udp_sock::send(udp_pkts& pkts)
{
	return send(pkts.get_pkts(), pkts.get_npkt());
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

int udp_sock::recv(udp_pkts& pkts)
{
	int ret = recv(pkts.get_pkts());
	if (ret >= 0)
		pkts.set_npkt(ret);
	return ret;
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
