#ifndef __MY_RFC1035_INCLUDE_H__
#define __MY_RFC1035_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#ifdef	ACL_UNIX
#include <netinet/in.h>
#endif

/* rfc1035 - DNS */
#define RFC1035_MAXHOSTNAMESZ 256

typedef struct rfc1035_rr {
    char name[RFC1035_MAXHOSTNAMESZ];
    unsigned short type;
    unsigned short tclass;	/* class */
    unsigned int ttl;
    unsigned short rdlength;
    char *rdata;
} rfc1035_rr;

typedef struct rfc1035_query {
    char name[RFC1035_MAXHOSTNAMESZ];
    unsigned short qtype;
    unsigned short qclass;
} rfc1035_query;

typedef struct rfc1035_message {
    unsigned short id;
    unsigned int qr:1;
    unsigned int opcode:4;
    unsigned int aa:1;
    unsigned int tc:1;
    unsigned int rd:1;
    unsigned int ra:1;
    unsigned int rcode:4;
    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount;
    unsigned short arcount;
    rfc1035_query *query;
    rfc1035_rr *answer;
} rfc1035_message;

const char *rfc1035Strerror(int errnum);
ssize_t rfc1035BuildAQuery(const char *hostname, char *buf, size_t sz,
	unsigned short qid, rfc1035_query * query);
ssize_t rfc1035BuildPTRQuery(const struct in_addr, char *buf, size_t sz,
	unsigned short qid, rfc1035_query * query);
void rfc1035SetQueryID(char *, unsigned short qid);
int rfc1035MessageUnpack(const char *buf, size_t sz,
	rfc1035_message ** answer);
int rfc1035QueryCompare(const rfc1035_query *, const rfc1035_query *);
void rfc1035MessageDestroy(rfc1035_message * message);
ssize_t rfc1035BuildAReply(const char *hostname, const ACL_ARGV *ip_argv,
	const char *dnsname, const char *dns_ip,
	unsigned short qid, char *buf, size_t sz);

extern int rfc1035_errno;
extern const char *rfc1035_error_message;

#define RFC1035_TYPE_A          1
#define RFC1035_TYPE_NS         2
#define RFC1035_TYPE_CNAME      5
#define RFC1035_TYPE_PTR        12
#define RFC1035_TYPE_AAAA       28
#define RFC1035_CLASS_IN        1

#ifdef __cplusplus
}
#endif

#endif

