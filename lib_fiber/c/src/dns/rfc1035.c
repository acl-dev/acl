#include "stdafx.h"

#ifdef	SYS_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <string.h>

#include "common/strops.h"
#include "common/msg.h"
#include "common/argv.h"
#include "rfc1035.h"

#define RFC1035_MAXLABELSZ	63
#define RFC1035_UNPACK_ERROR	15

#if 0
#define RFC1035_UNPACK_DEBUG  msg_error("unpack error at %s:%d", __FILE__,__LINE__)
#else
#define RFC1035_UNPACK_DEBUG  (void)0
#endif

/**
 * rfc1035_header_pack()
 *
 * Packs a RFC1035_header structure into a buffer.
 * Returns number of octets packed (should always be 12)
 */
static size_t rfc1035_header_pack(char *buf, size_t sz, RFC1035_MESSAGE * hdr)
{
	const char *myname = "rfc1035_header_pack";
	size_t off = 0;
	unsigned short s;
	unsigned short t;

	if (sz < 12) {
		msg_fatal("%s: sz(%d) < 12", myname, (int) sz);
	}

	s = htons(hdr->id);
	memcpy(buf + off, &s, sizeof(s));
	off += sizeof(s);
	t = 0;
	t |= hdr->qr << 15;
	t |= (hdr->opcode << 11);
	t |= (hdr->aa << 10);
	t |= (hdr->tc << 9);
	t |= (hdr->rd << 8);
	t |= (hdr->ra << 7);
	t |= hdr->rcode;
	s = htons(t);
	memcpy(buf + off, &s, sizeof(s));
	off += sizeof(s);
	s = htons(hdr->qdcount);
	memcpy(buf + off, &s, sizeof(s));
	off += sizeof(s);
	s = htons(hdr->ancount);
	memcpy(buf + off, &s, sizeof(s));
	off += sizeof(s);
	s = htons(hdr->nscount);
	memcpy(buf + off, &s, sizeof(s));
	off += sizeof(s);
	s = htons(hdr->arcount);
	memcpy(buf + off, &s, sizeof(s));
	off += sizeof(s);
	if (off != 12) {
		msg_fatal("%s: off(%d) != 12", myname, (int) off);
	}
	return off;
}

/**
 * rfc1035_label_pack()
 *
 * Packs a label into a buffer.  The format of
 * a label is one octet specifying the number of character
 * bytes to follow.  Labels must be smaller than 64 octets.
 * Returns number of octets packed.
 */
static int rfc1035_label_pack(char *buf, size_t sz, const char *label)
{
	const char *myname = "rfc1035_label_pack";
	int off = 0;
	size_t len = label ? strlen(label) : 0;

	if (label) {
		if (strchr(label, '.') != NULL) {
			msg_fatal("%s: '.' exist in label(%s)", myname, label);
		}
	}

	if (len > RFC1035_MAXLABELSZ) {
		len = RFC1035_MAXLABELSZ;
	}
	if (sz < len + 1) {
		msg_fatal("%s: sz(%d) < len(%d) + 1", myname, (int) sz, (int) len);
	}
	*(buf + off) = (char) len;
	off++;
	memcpy(buf + off, label, len);
	off += (int) len;
	return off;
}

/**
 * rfc1035_name_pack()
 *
 * Packs a name into a buffer.  Names are packed as a
 * sequence of labels, terminated with NULL label.
 * Note message compression is not supported here.
 * Returns number of octets packed.
 */
static int rfc1035_name_pack(char *buf, size_t sz, const char *name)
{
	const char *myname = "rfc1035_name_pack";
	int off = 0;
	char *copy, *ptr;
	char *t;

#ifdef SYS_WIN
	copy = _strdup(name);
#else
	copy = strdup(name);
#endif

	/*
	 * NOTE: use of strtok here makes names like foo....com valid.
	 */
	ptr = copy;
	for (t = mystrtok(&ptr, "."); t; t = mystrtok(&ptr, ".")) {
		off += rfc1035_label_pack(buf + off, sz - off, t);
	}
	free(copy);
	off += rfc1035_label_pack(buf + off, sz - off, NULL);
	if (off > (int) sz) {
		msg_fatal("%s: off(%d) > sz(%d)", myname, off, (int) sz);
	}
	return off;
}

/**
 * rfc1035_question_pack()
 *
 * Packs a QUESTION section of a message.
 * Returns number of octets packed.
 */
static int rfc1035_question_pack(char *buf, size_t sz, const char *name,
	unsigned short type, unsigned short tclass)
{
	const char *myname = "rfc1035_question_pack";
	int off = 0;
	unsigned short s;

	off += rfc1035_name_pack(buf + off, sz - off, name);

	s = htons(type);
	memcpy(buf + off, &s, sizeof(s));
	off += sizeof(s);

	s = htons(tclass);
	memcpy(buf + off, &s, sizeof(s));
	off += sizeof(s);

	if (off > (int) sz) {
		msg_error("%s: off(%d) > sz(%d)", myname, off, (int) sz);
		return 0;
	}

	return off;
}

/**
 * rfc1035_name_unpack()
 *
 * Unpacks a Name in a message buffer into a char*.
 * Note 'buf' points to the beginning of the whole message,
 * 'off' points to the spot where the Name begins, and 'sz'
 * is the size of the whole message.  'name' must be allocated
 * by the caller.
 *
 * Supports the RFC1035 message compression through recursion.
 *
 * Updates the new buffer offset.
 *
 * Returns 0 (success) or 1 (error)
 */
static int rfc1035_name_unpack(const char *buf, size_t sz, size_t *off,
	unsigned short *rdlength, char *name, size_t ns, int rdepth)
{
	const char *myname = "rfc1035_name_unpack";
	int no = 0;
	unsigned char c;
	size_t len;

	if (ns <= 0) {
		msg_error("%s: ns(%d) <= 0", myname, (int) ns);
		return -RFC1035_UNPACK_ERROR;
	}
	do {
		if (*off >= sz) {
			msg_error("%s: *off(%d) >= sz(%d)",
				myname, (int) *off, (int) sz);
			return -RFC1035_UNPACK_ERROR;
		}
		c = *(buf + (*off));
		if (c > 191) {
			/* blasted compression */
			unsigned short s;
			size_t ptr;
			if (rdepth > 64) {	/* infinite pointer loop */
				return 1;
			}
			memcpy(&s, buf + (*off), sizeof(s));
			s = ntohs(s);
			(*off) += sizeof(s);
			/* Sanity check */
			if ((*off) >= sz) {
				return 1;
			}
			ptr = s & 0x3FFF;
			/* Make sure the pointer is inside this message */
			if (ptr >= sz) {
				return 1;
			}
			return rfc1035_name_unpack(buf, sz, &ptr, rdlength,
				name + no, ns - no, rdepth + 1);
		} else if (c > RFC1035_MAXLABELSZ) {
			/*
			 * "(The 10 and 01 combinations are reserved for future use.)"
			 */
			return 1;
		} else {
			(*off)++;
			len = (size_t) c;
			if (len == 0) {
				break;
			}
			if (len > (ns - no - 1)) { /* label won't fit */
				return 1;
			}
			if ((*off) + len >= sz)	{ /* message is too short */
				return 1;
			}
			memcpy(name + no, buf + (*off), len);
			(*off) += (int) len;
			no += (int) len;
			*(name + (no++)) = '.';
			if (rdlength) {
				*rdlength += (unsigned short) len + 1;
			}
		}
	} while (c > 0 && no < (int) ns);

	if (no) {
		*(name + no - 1) = '\0';
	} else {
		*name = '\0';
	}
	/* make sure we didn't allow someone to overflow the name buffer */
	if (no > (int) ns) {
		msg_error("%s: no(%d) > ns(%d)", myname, no, (int) ns);
		return -RFC1035_UNPACK_ERROR;
	}
	return 0;
}

const char *rfc1035_strerror(int errnum)
{
	struct __ERRMSG{
		int   errnum;
		const char *msg;
	};
	static const struct __ERRMSG errmsg[] = {
		{ 0, "No error condition" },
		{ 1, "Format Error: The name server was unable to "
			 "interpret the query." },
		{ 2, "Server Failure: The name server was "
			 "unable to process this query." },
		{ 3, "Name Error: The domain name does not exist." },
		{ 4, "Not Implemented: The name server does "
				"not support the requested kind of query." },
		{ 5, "Refused: The name server refuses to "
				"perform the specified operation." },
		{ RFC1035_UNPACK_ERROR, "The DNS reply message is corrupt or could "
			"not be safely parsed." },
		{ -1, NULL },
	};
	const char *unknown = "Unknown Error";
	int   i;

	for (i = 0; errmsg[i].msg != NULL; i++) {
		if (errmsg[i].errnum == -errnum) {
			return (errmsg[i].msg);
		}
	}

	return unknown;
}

static void rfc1035_set_errno(int n)
{
	errno = n;
}

int rfc1035_query_compare(const RFC1035_QUERY * a, const RFC1035_QUERY * b)
{
	size_t la, lb;

	if (a->qtype != b->qtype) {
		return 1;
	}

	if (a->qclass != b->qclass) {
		return 1;
	}

	la = strlen(a->name);
	lb = strlen(b->name);

	if (la != lb) {
		/* Trim root label(s) */
		while (la > 0 && a->name[la - 1] == '.') {
			la--;
		}
		while (lb > 0 && b->name[lb - 1] == '.') {
			lb--;
		}
	}

	if (la != lb) {
		return 1;
	}

	return strncasecmp(a->name, b->name, la);
}

/**
 * rfc1035_rr_pack()
 *
 * Unpacks a RFC1035 Resource Record into 'RR' from a message buffer.
 * The caller must free RR->rdata!
 *
 * Updates the new message buffer offset.
 *
 * Returns > 0 (success) or 0 (error)
 */
static int rfc1035_rr_pack(const RFC1035_RR *rr, char *buf, size_t sz)
{
	const char *myname = "rfc1035_rr_pack";
	unsigned short s;
	unsigned int i;
	int off = 0, off_saved;

	off = rfc1035_name_pack(buf + off, sz, rr->name);
	s = htons(rr->type);
	memcpy(buf + off, &s,sizeof(s));
	off += sizeof(s);

	s = htons(rr->tclass);
	memcpy(buf + off, &s ,sizeof(s));
	off += sizeof(s);
    
	i = htonl(rr->ttl);
	memcpy(buf + off, &i ,sizeof(i));
	off += sizeof(i);

	switch (rr->type) {
	case RFC1035_TYPE_PTR:
	case RFC1035_TYPE_NS:
	case RFC1035_TYPE_CNAME:
	case RFC1035_TYPE_TXT:
		if (strlen(rr->rdata) > RFC1035_MAXHOSTNAMESZ) {
			return 0;
		}

		off_saved = off;
		off += sizeof(s);
		off += rfc1035_name_pack(buf + off, sz, rr->rdata);
		s = off - off_saved - (unsigned short) sizeof(s);
		s = htons(s);
		memcpy(buf + off_saved, &s ,sizeof(s));
		break;
	default:
		s = htons(rr->rdlength);
		memcpy(buf + off, &s ,sizeof(s));
		off += sizeof(s);
		memcpy(buf + off, rr->rdata, rr->rdlength);
		off += rr->rdlength;
		break;
	}

	if ((unsigned) off > sz) {
		msg_fatal("%s: off(%d) > sz(%d)", myname, off, (int) sz);
	}

	return off;
}

size_t rfc1035_build_query(const char *hostname, char *buf, size_t sz,
	unsigned short qid, unsigned short qtype, unsigned short qclass,
	RFC1035_QUERY *query)
{
	const char *myname = "rfc1035_build_query";
	RFC1035_MESSAGE h;
	size_t offset = 0;

	if (sz < 512) {
		msg_error("%s: sz(%d) < 512, too small", myname, (int) sz);
		return 0;
	}

	memset(&h, '\0', sizeof(h));
	h.id = qid;
	h.qr = 0;
	h.rd = 1;
	h.opcode = 0;		/* QUERY */
	h.qdcount = (unsigned int) 1;

	offset += rfc1035_header_pack(buf + offset, sz - offset, &h);
	offset += rfc1035_question_pack(buf + offset, sz - offset,
			hostname, qtype, qclass);
	if (query) {
		query->qtype  = qtype;
		query->qclass = qclass;
		SAFE_STRNCPY(query->name, hostname, sizeof(query->name));
	}

	if (offset > sz) {
		msg_fatal("%s: offset(%d) > sz(%d)", myname, (int) offset, (int) sz);
	}

	return offset;
}

size_t rfc1035_build_query4a(const char *hostname, char *buf, size_t sz,
	unsigned short qid, RFC1035_QUERY *query)
{
	return rfc1035_build_query(hostname, buf, sz, qid, RFC1035_TYPE_A,
			 RFC1035_CLASS_IN, query);
}

size_t rfc1035_build_query4aaaa(const char *hostname, char *buf, size_t sz,
	unsigned short qid, RFC1035_QUERY *query)
{
	return rfc1035_build_query(hostname, buf, sz, qid, RFC1035_TYPE_AAAA,
			RFC1035_CLASS_IN, query);
}

size_t rfc1035_build_query4mx(const char *hostname, char *buf, size_t sz,
	unsigned short qid, RFC1035_QUERY *query)
{
	return rfc1035_build_query(hostname, buf, sz, qid, RFC1035_TYPE_MX,
			RFC1035_CLASS_IN, query);
}

size_t rfc1035_build_query4ptr(const struct in_addr addr, char *buf,
	size_t sz, unsigned short qid, RFC1035_QUERY * query)
{
	const char *myname = "RFC1035BuildPTRQuery";
	RFC1035_MESSAGE h;
	size_t offset = 0;
	char rev[32];
	unsigned int i;

	memset(&h, '\0', sizeof(h));
	i = (unsigned int) ntohl(addr.s_addr);

#ifdef SYS_WIN
	_snprintf(rev, 32, "%u.%u.%u.%u.in-addr.arpa.",
#else
	snprintf(rev, 32, "%u.%u.%u.%u.in-addr.arpa.",
#endif
		i & 255, (i >> 8) & 255, (i >> 16) & 255, (i >> 24) & 255);

	h.id = qid;
	h.qr = 0;
	h.rd = 1;
	h.opcode = 0;		/* QUERY */
	h.qdcount = (unsigned int) 1;

	offset += rfc1035_header_pack(buf + offset, sz - offset, &h);
	offset += rfc1035_question_pack(buf + offset, sz - offset, rev,
			RFC1035_TYPE_PTR, RFC1035_CLASS_IN);

	if (query) {
		query->qtype = RFC1035_TYPE_PTR;
		query->qclass = RFC1035_CLASS_IN;
		SAFE_STRNCPY(query->name, rev, sizeof(query->name));
	}

	if (offset > sz) {
		msg_fatal("%s: offset(%d) > sz(%d)",
			myname, (int) offset, (int) sz);
	}

	return offset;
}

void rfc1035_set_query_id(char *buf, size_t sz, unsigned short qid)
{
	unsigned short s = htons(qid);

	if (sz > sizeof(s)) {
		sz = sizeof(s);
	}
	memcpy(buf, &s, sz);
}

/****************************************************************************/

/**
 * rfc1035_header_unpack()
 *
 * Unpacks a RFC1035 message header buffer into the header fields
 * of the RFC1035_MESSAGE structure.
 *
 * Updates the buffer offset, which is the same as number of
 * octects unpacked since the header starts at offset 0.
 *
 * Returns 0 (success) or 1 (error)
 */
static int rfc1035_header_unpack(const char *buf, size_t sz, size_t *off,
	RFC1035_MESSAGE *h)
{
	const char *myname = "rfc1035_header_unpack";
	unsigned short s;
	unsigned short t;

	if (*off != 0) {
		msg_error("%s: *off(%d) != 0", myname, (int) *off);
		return -RFC1035_UNPACK_ERROR;
	}

	/*
	 * The header is 12 octets.  This is a bogus message if the size
	 * is less than that.
	 */
	if (sz < 12) {
		return 1;
	}

	memcpy(&s, buf + (*off), sizeof(s));
	(*off) += sizeof(s);

	h->id = ntohs(s);
	memcpy(&s, buf + (*off), sizeof(s));
	(*off) += sizeof(s);

	t = ntohs(s);
	h->qr = (t >> 15) & 0x01;
	h->opcode = (t >> 11) & 0x0F;
	h->aa = (t >> 10) & 0x01;
	h->tc = (t >> 9) & 0x01;
	h->rd = (t >> 8) & 0x01;
	h->ra = (t >> 7) & 0x01;

	/* We might want to check that the reserved 'Z' bits (6-4) are
	 * all zero as per RFC 1035.  If not the message should be
	 * rejected.
	 */
	h->rcode = t & 0x0F;

	memcpy(&s, buf + (*off), sizeof(s));
	(*off) += sizeof(s);
	h->qdcount = ntohs(s);

	memcpy(&s, buf + (*off), sizeof(s));
	(*off) += sizeof(s);
	h->ancount = ntohs(s);

	memcpy(&s, buf + (*off), sizeof(s));
	(*off) += sizeof(s);
	h->nscount = ntohs(s);

	memcpy(&s, buf + (*off), sizeof(s));
	(*off) += sizeof(s);
	h->arcount = ntohs(s);

	if (*off != 12) {
		msg_error("%s: *off(%d) != 12", myname, (int) *off);
		return -RFC1035_UNPACK_ERROR;
	}
	return 0;
}

/**
 * rfc1035_query_unpack()
 *
 * Unpacks a RFC1035 Query Record into 'query' from a message buffer.
 *
 * Updates the new message buffer offset.
 *
 * Returns 0 (success) or 1 (error)
*/
static int rfc1035_query_unpack(const char *buf, size_t sz, size_t *off,
	RFC1035_QUERY *query)
{
	unsigned short s;

	if (rfc1035_name_unpack(buf, sz, off, NULL, query->name,
		RFC1035_MAXHOSTNAMESZ, 0)) {

		RFC1035_UNPACK_DEBUG;
		memset(query, '\0', sizeof(*query));
		return 1;
	}

	if (*off + 4 > sz) {
		RFC1035_UNPACK_DEBUG;
		memset(query, '\0', sizeof(*query));
		return 1;
	}

	memcpy(&s, buf + *off, 2);
	*off += 2;
	query->qtype = ntohs(s);

	memcpy(&s, buf + *off, 2);
	*off += 2;
	query->qclass = ntohs(s);

	return 0;
}

/**
 * rfc1035_rr_unpack()
 *
 * Unpacks a RFC1035 Resource Record into 'RR' from a message buffer.
 * The caller must free RR->rdata!
 *
 * Updates the new message buffer offset.
 *
 * Returns 0 (success) or 1 (error)
 */
static int rfc1035_rr_unpack(const char *buf, size_t sz, size_t *off, RFC1035_RR *RR)
{
	const char *myname = "rfc1035_rr_unpack";
	unsigned short s;
	unsigned int i;
	unsigned short rdlength;
	size_t rdata_off;

	if (rfc1035_name_unpack(buf, sz, off, NULL, RR->name,
		RFC1035_MAXHOSTNAMESZ, 0)) {

		RFC1035_UNPACK_DEBUG;
		memset(RR, '\0', sizeof(*RR));
		return 1;
	}

	/* Make sure the remaining message has enough octets for the
	* rest of the RR fields.
	*/
	if ((*off) + 10 > sz) {
		RFC1035_UNPACK_DEBUG;
		memset(RR, '\0', sizeof(*RR));
		return 1;
	}

	memcpy(&s, buf + (*off), sizeof(s));
	(*off) += sizeof(s);
	RR->type = ntohs(s);

	memcpy(&s, buf + (*off), sizeof(s));
	(*off) += sizeof(s);
	RR->tclass = ntohs(s);

	memcpy(&i, buf + (*off), sizeof(i));
	(*off) += sizeof(i);
	RR->ttl = ntohl(i);

	memcpy(&s, buf + (*off), sizeof(s));
	(*off) += sizeof(s);
	rdlength = ntohs(s);

	if ((*off) + rdlength > sz) {
		/* We got a truncated packet.  'dnscache' truncates UDP
		 * replies at 512 octets, as per RFC 1035.
		 */
		RFC1035_UNPACK_DEBUG;
		memset(RR, '\0', sizeof(*RR));
		return 1;
	}

	RR->rdlength = rdlength;

	switch (RR->type) {
	case RFC1035_TYPE_CNAME:
	case RFC1035_TYPE_NS:
	case RFC1035_TYPE_TXT:
	case RFC1035_TYPE_PTR:
	case RFC1035_TYPE_WKS:
		RR->rdata = (char*) malloc(RFC1035_MAXHOSTNAMESZ);
		rdata_off = *off;
		RR->rdlength = 0;	/* Filled in by rfc1035_name_unpack */

		if (rfc1035_name_unpack(buf, sz, &rdata_off, &RR->rdlength,
			RR->rdata, RFC1035_MAXHOSTNAMESZ, 0)) {
			return 1;
		}

		if (rdata_off > ((*off) + rdlength)) {
			/* This probably doesn't happen for valid packets, but
			 * I want to make sure that NameUnpack doesn't go beyond
			 * the RDATA area.
			 */
			RFC1035_UNPACK_DEBUG;
			free(RR->rdata);
			memset(RR, '\0', sizeof(*RR));
			return 1;
		}
		break;
	case RFC1035_TYPE_A:
	case RFC1035_TYPE_AAAA:
	case RFC1035_TYPE_MX:
	default:
		RR->rdata = (char*) malloc(rdlength);
		memcpy(RR->rdata, buf + (*off), rdlength);
		break;
	}

	(*off) += rdlength;
	if (*off > sz) {
		msg_error("%s: *off(%d) > sz(%d)",
			myname, (int) *off, (int) sz);
		return -RFC1035_UNPACK_ERROR;
	}

	return 0;
}

static RFC1035_RR *rfc1035_unpack2rr(const char *buf, size_t sz,
	size_t *off, size_t count, int *nr)
{
	size_t i;
	RFC1035_RR *rr;

	*nr = 0;

	if (count == 0) {
		return NULL;
	}

	rr = (RFC1035_RR*) calloc(count, sizeof(RFC1035_RR));
	for (i = 0; i < count; i++) {
		if (*off >= sz) {	/* corrupt packet */
			RFC1035_UNPACK_DEBUG;
			break;
		}

		if (rfc1035_rr_unpack(buf, sz, off, &rr[i])) {
			RFC1035_UNPACK_DEBUG;
			break;
		}
		(*nr)++;
	}

	if (*nr == 0) {
		free(rr);
		return NULL;
	}

	return rr;
}

RFC1035_MESSAGE *rfc1035_response_unpack(const char *buf, size_t sz)
{
	int i, nr;
	size_t off = 0;
	RFC1035_MESSAGE *msg;

	errno = 0;
	msg = (RFC1035_MESSAGE*) calloc(1, sizeof(*msg));

	if (rfc1035_header_unpack(buf + off, sz - off, &off, msg)) {
		RFC1035_UNPACK_DEBUG;
		rfc1035_set_errno(RFC1035_UNPACK_ERROR);
		rfc1035_message_destroy(msg);
		return NULL;
	}

	if (msg->rcode) {
		RFC1035_UNPACK_DEBUG;
		rfc1035_set_errno((int) msg->rcode);
		rfc1035_message_destroy(msg);
		return NULL;
	}

	if (msg->ancount == 0) {
		rfc1035_message_destroy(msg);
		return NULL;
	}

	if (msg->qdcount != 1) {
		/* This can not be an answer to our queries.. */
		RFC1035_UNPACK_DEBUG;
		rfc1035_set_errno(RFC1035_UNPACK_ERROR);
		rfc1035_message_destroy(msg);
		return NULL;
	}

	msg->query = (RFC1035_QUERY*) calloc((int) msg->qdcount,
			sizeof(RFC1035_QUERY));

	for (i = 0; i < (int) msg->qdcount; i++) {
		if (rfc1035_query_unpack(buf, sz, &off, &msg->query[i])) {
			RFC1035_UNPACK_DEBUG;
			rfc1035_set_errno(RFC1035_UNPACK_ERROR);
			rfc1035_message_destroy(msg);
			return NULL;
		}
	}

	msg->answer = rfc1035_unpack2rr(buf, sz, &off, msg->ancount, &nr);
	msg->ancount = (unsigned short) nr;  /* reset the valid ancount */

	if (msg->answer == NULL) {
		/* we expected to unpack some answers (ancount != 0), but
		 * didn't actually get any.
		 */
		RFC1035_UNPACK_DEBUG;
		rfc1035_message_destroy(msg);
		rfc1035_set_errno(RFC1035_UNPACK_ERROR);
		return NULL;
	}

	if (msg->nscount > 0) {
		msg->authority = rfc1035_unpack2rr(buf, sz, &off,
			msg->nscount, &nr);
		msg->nscount = (unsigned short) nr;
		if (msg->authority == NULL) {
			RFC1035_UNPACK_DEBUG;
			rfc1035_message_destroy(msg);
			rfc1035_set_errno(RFC1035_UNPACK_ERROR);
			return NULL;
		}
	}

	if (msg->arcount > 0) {
		msg->additional = rfc1035_unpack2rr(buf, sz, &off,
			msg->arcount, &nr);
		msg->arcount = (unsigned short) nr;
		if (msg->additional == NULL) {
			RFC1035_UNPACK_DEBUG;
			rfc1035_message_destroy(msg);
			rfc1035_set_errno(RFC1035_UNPACK_ERROR);
			return NULL;
		}
	}

	return msg;
}

RFC1035_MESSAGE *rfc1035_request_unpack(const char *buf, size_t sz)
{
	int i;
	size_t off = 0;
	RFC1035_MESSAGE *msg;

	errno = 0;
	msg = (RFC1035_MESSAGE*) calloc(1, sizeof(*msg));

	if (rfc1035_header_unpack(buf + off, sz - off, &off, msg)) {
		RFC1035_UNPACK_DEBUG;
		rfc1035_set_errno(RFC1035_UNPACK_ERROR);
		rfc1035_message_destroy(msg);
		return NULL;
	}

	if (msg->rcode) {
		RFC1035_UNPACK_DEBUG;
		rfc1035_set_errno((int) msg->rcode);
		rfc1035_message_destroy(msg);
		return NULL;
	}

	if (msg->qdcount != 1) {
		/* This can not be an answer to our queries.. */
		RFC1035_UNPACK_DEBUG;
		rfc1035_set_errno(RFC1035_UNPACK_ERROR);
		rfc1035_message_destroy(msg);
		return NULL;
	}

	msg->query = (RFC1035_QUERY*) calloc((int) msg->qdcount,
			sizeof(RFC1035_QUERY));

	for (i = 0; i < (int) msg->qdcount; i++) {
		if (rfc1035_query_unpack(buf, sz, &off, &msg->query[i])) {
			RFC1035_UNPACK_DEBUG;
			rfc1035_set_errno(RFC1035_UNPACK_ERROR);
			rfc1035_message_destroy(msg);
		}
	}

	return msg;
}

/****************************************************************************/

static void rfc1035_rr_destroy(RFC1035_RR * rr, int n)
{
	const char *myname = "rfc1035_rr_destroy";

	if (rr == NULL) {
		return;
	}

	if (n <= 0) {
		msg_error("%s: n(%d) <= 0", myname, n);
		free(rr);
		return;
	}

	while (n--) {
		if (rr[n].rdata) {
			free(rr[n].rdata);
		}
	}

	free(rr);
}

void rfc1035_message_destroy(RFC1035_MESSAGE *msg)
{
	if (!msg) {
		return;
	}

	if (msg->query) {
		free(msg->query);
	}

	if (msg->answer) {
		rfc1035_rr_destroy(msg->answer, msg->ancount);
	}

	if (msg->authority) {
		rfc1035_rr_destroy(msg->authority, msg->nscount);
	}

	if (msg->additional) {
		rfc1035_rr_destroy(msg->additional, msg->arcount);
	}

	free(msg);
}

/****************************************************************************/

size_t rfc1035_build_reply4a(const char *hostname, const ARGV *ips,
	const char *domain_root, const char *dnsname, const char *dnsip,
	unsigned short qid, char *buf, size_t sz)
{
	RFC1035_REPLY reply;

	memset(&reply, 0, sizeof(reply));
	reply.hostname = hostname;
	reply.ips = ips;
	reply.domain_root = domain_root;
	reply.dns_name = dnsname;
	reply.dns_ip = dnsip;
	reply.ip_type = RFC1035_TYPE_A;
	reply.ttl = 600;
	reply.qid = qid;

	return rfc1035_build_reply(&reply, buf, sz);
}

size_t rfc1035_build_reply4aaaa(const char *hostname, const ARGV *ips,
	const char *domain_root, const char *dnsname, const char *dnsip,
	unsigned short qid, char *buf, size_t sz)
{
	RFC1035_REPLY reply;

	memset(&reply, 0, sizeof(reply));
	reply.hostname = hostname;
	reply.ips = ips;
	reply.domain_root = domain_root;
	reply.dns_name = dnsname;
	reply.dns_ip = dnsip;
	reply.ip_type = RFC1035_TYPE_AAAA;
	reply.ttl = 600;
	reply.qid = qid;

	return rfc1035_build_reply(&reply, buf, sz);
}

static size_t save_addr2rr(int type, const char *src, RFC1035_RR *rr)
{
	if (type == RFC1035_TYPE_A) {
		unsigned int nip;
		rr->rdlength = 4;
		nip = inet_addr(src);
		rr->rdata = (char*) calloc(1, rr->rdlength);
		memcpy(rr->rdata, &nip, rr->rdlength);

		return rr->rdlength;
#ifdef AF_INET6
	} else if (type == RFC1035_TYPE_AAAA) {
		char buf[256], *ptr;
		struct sockaddr_in6 in6;

		SAFE_STRNCPY(buf, src, sizeof(buf));
		/* when '%' was appended to the IPV6's addr */
		if ((ptr = strrchr(buf, '%'))) {
			*ptr++ = 0;
		}

		memset(&in6, 0, sizeof(struct sockaddr_in6));
		in6.sin6_family = AF_INET6;
		in6.sin6_port   = htons(0);

#if defined(SYS_UNIX) || (defined(SYS_WIN) && _MSC_VER >= 1600)
		if (ptr && *ptr && !(in6.sin6_scope_id = if_nametoindex(ptr))) {
			msg_error("%s(%d): if_nametoindex error %s",
				__FUNCTION__, __LINE__, last_serror());
			return 0;
		}
#endif
		if (inet_pton(AF_INET6, buf, &in6.sin6_addr) == 0) {
			msg_error("%s(%d): invalid addr=%s",
				__FUNCTION__, __LINE__, src);
			return 0;
		}

		rr->rdlength = sizeof(in6.sin6_addr);
		rr->rdata = (char*) calloc(1, rr->rdlength);
		memcpy(rr->rdata, &in6.sin6_addr, rr->rdlength);

		return rr->rdlength;
	}
#endif
	else {
		msg_error("%s(%d): not support type=%d",
		      __FUNCTION__, __LINE__, type);
		return 0;
	}
}

#define SAFE_FREE(x) do { if ((x)) free ((x)); } while(0)

size_t rfc1035_build_reply(const RFC1035_REPLY *reply, char *buf, size_t sz)
{
	RFC1035_MESSAGE h;
	RFC1035_RR rr;
	size_t offset = 0;
	int   i;

	if (reply->ips == NULL || reply->ips->argc <= 0) {
		msg_error("ips null");
		return 0;
	}

	memset(&h, '\0', sizeof(h));
	h.id = reply->qid;
	h.qr = 1;		/* response */
	h.opcode = 0;		/* QUERY */
	h.aa = 0;
	h.tc = 0;
	h.rd = 1;
	h.ra = 0;
	h.rcode = 0;
	h.qdcount = 1;
	h.ancount = reply->ips->argc;
	h.nscount = (reply->dns_name && *reply->dns_name) ? 1 : 0;
	h.arcount = (h.nscount && reply->dns_ip && *reply->dns_ip) ? 1 : 0;

	offset += rfc1035_header_pack(buf + offset, sz - offset, &h);
	offset += rfc1035_question_pack(buf + offset, sz - offset,
			reply->hostname, reply->ip_type, RFC1035_CLASS_IN);

	for (i = 0; i < reply->ips->argc; i++) {
		memset(&rr, 0, sizeof(rr));
		SAFE_STRNCPY(rr.name, reply->hostname, sizeof(rr.name));
		rr.type = reply->ip_type;
		rr.tclass = RFC1035_CLASS_IN;
		rr.ttl = reply->ttl;

		if (!save_addr2rr(reply->ip_type, reply->ips->argv[i], &rr)) {
			msg_error("%s(%d): invalid ip=%s",
				__FUNCTION__ , __LINE__, reply->ips->argv[i]);
			return 0;
		}

		offset += rfc1035_rr_pack(&rr, buf + offset, sz - offset);
		SAFE_FREE(rr.rdata);
	}

	if (h.nscount) {
		memset(&rr, 0, sizeof(rr));
		SAFE_STRNCPY(rr.name, reply->domain_root, sizeof(rr.name));
		rr.type = RFC1035_TYPE_NS;
		rr.tclass = RFC1035_CLASS_IN;
		rr.ttl = reply->ttl;
		rr.rdlength = (unsigned short) strlen(reply->dns_name);
#ifdef SYS_WIN
		rr.rdata = _strdup(reply->dns_name);
#else
		rr.rdata = strdup(reply->dns_name);
#endif
		offset += rfc1035_rr_pack(&rr, buf + offset, sz - offset);
		SAFE_FREE(rr.rdata);
	}

	if (h.arcount) {
		memset(&rr, 0, sizeof(rr));
		SAFE_STRNCPY(rr.name, reply->dns_name, sizeof(rr.name));
		rr.type = reply->ip_type;
		rr.tclass = RFC1035_CLASS_IN;
		rr.ttl = reply->ttl;

		if (!save_addr2rr(reply->ip_type, reply->dns_ip, &rr)) {
			msg_error("%s(%d): invalid ip=%s",
				__FUNCTION__ , __LINE__, reply->dns_ip);
			return 0;
		}

		offset += rfc1035_rr_pack(&rr, buf + offset, sz - offset);
		SAFE_FREE(rr.rdata);
	}

	return offset;
}

/****************************************************************************/
