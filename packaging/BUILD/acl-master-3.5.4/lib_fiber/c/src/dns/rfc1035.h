#ifndef __RFC1035_INCLUDE_H__
#define __RFC1035_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	ACL_UNIX
#include <netinet/in.h>
#endif

struct ARGV;

/* RFC1035 - DNS */
#define RFC1035_MAXHOSTNAMESZ 256

typedef struct RFC1035_RR {
    char name[RFC1035_MAXHOSTNAMESZ];
    unsigned short type;
    unsigned short tclass;	/* class */
    unsigned int ttl;
    unsigned short rdlength;
    char *rdata;
} RFC1035_RR;

typedef struct RFC1035_QUERY {
    char name[RFC1035_MAXHOSTNAMESZ];
    unsigned short qtype;
    unsigned short qclass;
} RFC1035_QUERY;

typedef struct RFC1035_MESSAGE {
    unsigned short id;		/* A 16 bit identifier */
    unsigned int qr:1;		/* This message is a query (0), or a response (1) */
    unsigned int opcode:4;	/* Query kind:
 				 * 0: A standard query (QUERY)
 				 * 1: An inverse query (IQUERY)
 				 * 2: A server status request (STATUS)
 				 * 3-15: Reserved for future use
 				 */
    unsigned int aa:1;		/* Response: Authoritative Answer */
    unsigned int tc:1;		/* Response: Specifies that this message was truncated */
    unsigned int rd:1;		/* Query->Response: Recursion Desired, optional */
    unsigned int ra:1;		/* Response: Recursion Available  */
    unsigned int z:4;		/* Reserved for future use */
    unsigned int rcode:4;	/* Response code:
 				 * 0: No error condition
 				 * 1: Query format error
 				 * 2: Server failure
 				 * 3: Name Error, domain in the query not exist
 				 * 4: Not implement in server side
 				 * 5: Refused by the server
 				 */
    unsigned short qdcount;	/* the number of entries in the question section */
    unsigned short ancount;	/* the number of resource records in the answer section */
    unsigned short nscount;	/* the number of name server resource records in the authority records */
    unsigned short arcount;	/* the number of resource records in the additional records section */
    RFC1035_QUERY *query;
    RFC1035_RR    *answer;
    RFC1035_RR    *authority;
    RFC1035_RR    *additional;
} RFC1035_MESSAGE;

/**
 * Get error description with the specifed error number
 * @param errnum {int}
 * @return {const char*}
 */
const char *rfc1035_strerror(int errnum);

/**
 * Builds a message buffer with a QUESTION to lookup A records
 * for a hostname.  Caller must allocate 'buf' which should
 * probably be at least 512 octets.  The 'szp' initially
 * specifies the size of the buffer, on return it contains
 * the size of the message (i.e. how much to write).
 * @param hostname {const char*} the hostname to be resolved
 * @param buf {char*} the buffer for holding the query content
 * @param sz {size_t} the buffer's size
 * @param qid {unsigned short} the unique ID number for thie quering
 * @param query {RFC1035_QUERY*} if not null, it's qtype, qclass and
 * 	hostname will be set.
 * @return {size_t} return the size of the query in the buf, 0 return
 * 	if some error happened.
 */
size_t rfc1035_build_query4a(const char *hostname, char *buf, size_t sz,
	unsigned short qid, RFC1035_QUERY *query);

/**
 * Builds a message buffer with a QUESTION to lookup AAAA records
 * for a hostname.
 * @param hostname {const char*}
 * @param buf  {char*}
 * @param sz {size_t}
 * @param qid {unsigned short}
 * @param query {RFC1035_QUERY*}
 * @return {size_t}
 */
size_t rfc1035_build_query4aaaa(const char *hostname, char *buf, size_t sz,
	unsigned short qid, RFC1035_QUERY *query);

/**
 * Builds a message buffer with a QUESTION to lookup MX records
 * for a hostname.
 * @param hostname {const char*}
 * @param buf {char*}
 * @param sz {size_t}
 * @param qid {unsigned short}
 * @param query {RFC1035_QUERY*}
 * @return {size_t}
 */
size_t rfc1035_build_query4mx(const char *hostname, char *buf, size_t sz,
	unsigned short qid, RFC1035_QUERY *query);

size_t rfc1035_build_query(const char *hostname, char *buf, size_t sz,
	unsigned short qid, unsigned short qtype, unsigned short qclass,
	RFC1035_QUERY *query);

/**
 * Builds a message buffer with a QUESTION to lookup PTR records
 * for an address.  Caller must allocate 'buf' which should
 * probably be at least 512 octets.  The 'szp' initially
 * specifies the size of the buffer, on return it contains
 * the size of the message (i.e. how much to write).
 * @param addr {const struct in_addr} the addr to resolve for name
 * @param buf {char*} hold the query package
 * @param sz {size_t} the buf's size at least 512 octets
 * @param qid {unsigned short} the query ID
 * @param query {RFC1035_QUERY*} if no null, it's some member variable
 * @return {size_t} Returns the size of the query in the buff
*/
size_t rfc1035_build_query4ptr(const struct in_addr addr, char *buf, size_t sz,
	unsigned short qid, RFC1035_QUERY *query);

/**
 * We're going to retry a former query, but we
 * just need a new ID for it.  Lucky for us ID
 * is the first field in the message buffer.
 * @param buf {char*} hold the result
 * @param sz {size_t} the buf's size
 * @param qid {unsigned short} the query ID
 */
void rfc1035_set_query_id(char *buf, size_t sz, unsigned short qid);

/**
 * Compares two RFC1035_QUERY entries
 * @param a {const RFC1035_QUERY *}
 * @param b {const RFC1035_QUERY *}
 * @return Returns 0 (equal) or !=0 (different)
 */
int rfc1035_query_compare(const RFC1035_QUERY *a, const RFC1035_QUERY *b);

/**
 * Takes the contents of a DNS request and fills in an array
 * of resource record structures.  The records array is allocated
 * here, and should be freed by calling rfc1035_messager_destroy().
 * @param buf {const char*} the data of the DNS reply
 * @param sz {size_t} the buf's size
 * @return {RFC1035_MESSAGE*} return the parsing result
 */
RFC1035_MESSAGE *rfc1035_request_unpack(const char *buf, size_t sz);

/**
 * Takes the contents of a DNS reply and fills in an array
 * of resource record structures.  The records array is allocated
 * here, and should be freed by calling rfc1035_messager_destroy().
 * @param buf {const char*} the data of the DNS reply
 * @param sz {size_t} the buf's size
 * @return {RFC1035_MESSAGE*} return the parsing result
 */
RFC1035_MESSAGE *rfc1035_response_unpack(const char *buf, size_t sz);

/**
 * destroy and free the message created by RFC1035MessageUnpack()
 * @param message {RFC1035_MESSAGE*}
 */
void rfc1035_message_destroy(RFC1035_MESSAGE *message);

/**
 * Builds a response message for the query client.
 * @param hostname {const char*} the name to be resolved
 * @param ips {const ARGV*} the ip list of the hostname
 * @param domain_root {const char*}
 * @param dnsname  {const char*}
 * @param dnsip  {const char*}
 * @param qid {unsigned short}
 * @param buf {char*} hold the result
 * @param sz {size_t} the buf's size
 * @return {size_t} content size in buf
 */
size_t rfc1035_build_reply4a(const char *hostname, const ARGV *ips,
	const char *domain_root, const char *dnsname, const char *dnsip,
	unsigned short qid, char *buf, size_t sz);

size_t rfc1035_build_reply4aaaa(const char *hostname, const ARGV *ips,
	const char *domain_root, const char *dnsname, const char *dnsip,
	unsigned short qid, char *buf, size_t sz);

typedef struct RFC1035_REPLY {
	const char *hostname;
	const ARGV *ips;
	const char *domain_root;
	const char *dns_name;
	const char *dns_ip;
	int ip_type;
	int ttl;
	unsigned short qid;
} RFC1035_REPLY;

size_t rfc1035_build_reply(const RFC1035_REPLY *reply, char *buf, size_t sz);

#define RFC1035_TYPE_A		1	/* a host address */
#define RFC1035_TYPE_NS		2	/* an authoritative name server */
#define RFC1035_TYPE_MD		3	/* a mail destination (Obsolete - use MX) */
#define RFC1035_TYPE_MF		4	/* a mail forwarder (Obsolete - use MX) */
#define RFC1035_TYPE_CNAME	5	/* the canonical name for an alias */
#define RFC1035_TYPE_SOA	6	/* marks the start of a zone of authority */
#define RFC1035_TYPE_MB		7	/* a mailbox domain name (EXPERIMENTAL) */
#define RFC1035_TYPE_MG		8	/* a mail group member (EXPERIMENTAL) */
#define RFC1035_TYPE_MR		9	/* a mail rename domain name (EXPERIMENTAL) */
#define RFC1035_TYPE_NULL	10	/* a null RR (EXPERIMENTAL) */
#define RFC1035_TYPE_WKS	11	/* a well known service description */
#define RFC1035_TYPE_PTR	12	/* a domain name pointer */
#define RFC1035_TYPE_HINFO	13	/* host information */
#define RFC1035_TYPE_MINFO	14	/* mailbox or mail list information */
#define RFC1035_TYPE_MX		15	/* mail exchange */
#define RFC1035_TYPE_TXT	16	/* text strings */
#define RFC1035_TYPE_AAAA	28	/* a IPv6 address of host */
#define RFC1035_TYPE_AXFR	252	/* a request for a transfer of an entire zone */
#define RFC1035_TYPE_MAILB	253	/* a request for mailbox-related records (MB, MG or MR) */
#define RFC1035_TYPE_MAILA	253	/* a request for mail agent RRs (Obsolete - see MX) */
#define RFC1035_TYPE_ALL	255	/* a request for all records */

#define RFC1035_CLASS_IN        1	/* the Internet */
#define RFC1035_CLASS_CS        2	/* the CSNET class (Obsolete - used only for examples in some obsolete RFCs */
#define RFC1035_CLASS_CH        3	/* the CHAOS class */
#define RFC1035_CLASS_HS        4	/* Hesiod [Dyer 87] */

#ifdef __cplusplus
}
#endif

#endif

