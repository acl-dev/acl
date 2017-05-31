/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Thu 30 May 2017 05:26:42 PM CST
 */

#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/un.h>
#include <vector>

typedef struct SOCK_ADDR {
	union {
		struct sockaddr_storage ss;
#ifdef AF_INET6
		struct sockaddr_in6 in6;
#endif
		struct sockaddr_in in;
#ifdef ACL_UNIX
		struct sockaddr_un un;
#endif
		struct sockaddr sa;
	} sa;
} SOCK_ADDR;

class udp_sock;

class udp_pkt
{
public:
	udp_pkt(size_t size = 1460);
	~udp_pkt(void);

	bool set_data(const void* data, size_t len);
	bool set_peer(const char* addr);

	void* get_data(void) const
	{
		return iov_.iov_base;
	}

	size_t get_dlen(void) const
	{
		return iov_.iov_len;
	}

	int get_port(void) const;
	const char* get_ip(char* buf, size_t len) const;

private:
	friend class udp_sock;

	struct iovec iov_;
	const size_t size_;
	SOCK_ADDR    addr_;
	socklen_t    addr_len_;
};

class udp_pkts
{
public:
	udp_pkts(size_t n);
	~udp_pkts(void);

	udp_pkt* operator[](size_t i);

	std::vector<udp_pkt*>& get_pkts(void)
	{
		return pkts_;
	}

private:
	std::vector<udp_pkt*> pkts_;
};

class udp_sock
{
public:
	udp_sock(void);
	~udp_sock(void);

	bool bind(const char* addr);
	bool client_open(const char* local, const char* peer);
	bool server_open(const char* local);

	ssize_t send(const void* data, size_t len);
	ssize_t recv(void* buf, size_t len);

	int recv(std::vector<udp_pkt*>& pkts);
	int send(std::vector<udp_pkt*>& pkts, size_t max);

private:
	int             fd_;

	SOCK_ADDR       sa_local_;
	socklen_t       sa_local_len_;

	SOCK_ADDR       sa_peer_;
	socklen_t       sa_peer_len_;

	struct mmsghdr* msgvec_;
	size_t          vlen_;
};
