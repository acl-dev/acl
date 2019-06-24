#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/un.h>

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

typedef struct SOCK_PKT {
	struct iovec    iov;
	SOCK_ADDR       addr;
	socklen_t       addr_len;
} SOCK_PKT;

typedef struct SOCK_UDP {
	int             fd;
	SOCK_ADDR       sa_local;
	socklen_t       sa_local_len;

	SOCK_ADDR       sa_peer;
	socklen_t       sa_peer_len;

	SOCK_PKT       *pkts;
	size_t          pkts_cnt;

	struct mmsghdr *msgvec;
	size_t          vlen;
} SOCK_UDP; 

SOCK_UDP *udp_client_open(const char* local, const char *peer);
SOCK_UDP *udp_server_open(const char* local);

void udp_close(SOCK_UDP *sock);
int  udp_send(SOCK_UDP *sock, const void *data, size_t len);
int  udp_read(SOCK_UDP *sock, void *buf, size_t size);

void udp_pkt_set_buf(SOCK_PKT *pkt, char *buf, size_t len);
int  udp_pkt_set_peer(SOCK_PKT *pkt, const char *addr);
int  udp_mread(SOCK_UDP *sock, SOCK_PKT pkts[], size_t pkts_cnt);
int  udp_msend(SOCK_UDP *sock, SOCK_PKT pkts[], size_t pkts_cnt);

int  pkt_port(SOCK_PKT *pkt);
int  udp_port(SOCK_UDP *sock, size_t i);
const char *pkt_ip(SOCK_PKT *pkt, char *buf, size_t size);
const char *udp_ip(SOCK_UDP *sock, size_t i, char *buf, size_t size);

#define PKT_IOV_DAT(pp)               ((pp)->iov.iov_base)
#define PKT_IOV_LEN(pp)               ((pp)->iov.iov_len)
#define SOCK_PKT_LEN(ss, ii)          ((ss)->msgvec[(ii)].msg_len)
