#include "stdafx.h"
#include "sock_addr.h"
#include "udp_pkt.h"

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
	if (addr == NULL || *addr == 0)
	{
		logger_error("addr null");
		return false;
	}

	struct addrinfo *peer_res0 = host_addrinfo(addr);

	if (peer_res0 == NULL)
	{
		logger_error("invalid addr=%s", addr);
		return false;
	}

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

const char *udp_pkt::get_ip(void) const
{
	if (addr_.sa.sa.sa_family == AF_INET)
	{
		const struct sockaddr_in *in = &addr_.sa.in;
		return inet_ntop(in->sin_family, &in->sin_addr,
			const_cast<udp_pkt*>(this)->ipbuf_,
			sizeof(ipbuf_));
	}
#ifdef AF_INET6
	else if (addr_.sa.sa.sa_family == AF_INET6)
	{
		const struct sockaddr_in6 *in = &addr_.sa.in6;
		return inet_ntop(in->sin6_family, &in->sin6_addr,
			const_cast<udp_pkt*>(this)->ipbuf_,
			sizeof(ipbuf_));
	}
#endif
	else
	{
		logger_error("unkown sa_family=%d", addr_.sa.sa.sa_family);
		return NULL;
	}
}

udp_pkts::udp_pkts(size_t max)
	: max_(max)
	, npkt_(max)
{
	acl_assert(max > 0);

	for (size_t i = 0; i < max; i++)
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

void udp_pkts::set_npkt(size_t n)
{
	npkt_ = n;
}
