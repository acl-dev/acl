
#include "stdafx.h"
#include <stdio.h>  /* for snprintf */

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "rfc1035.h"

#define RFC1035_MAXLABELSZ 63
#define rfc1035_unpack_error 15

int rfc1035_errno;
const char *rfc1035_error_message;

#if 0
#define RFC1035_UNPACK_DEBUG  acl_msg_error("unpack error at %s:%d", __FILE__,__LINE__)
#else
#define RFC1035_UNPACK_DEBUG  (void)0
#endif

/*
* rfc1035HeaderPack()
* 
* Packs a rfc1035_header structure into a buffer.
* Returns number of octets packed (should always be 12)
*/
static int rfc1035HeaderPack(char *buf, size_t sz, rfc1035_message * hdr)
{
	const char *myname = "rfc1035HeaderPack";
	int off = 0;
	unsigned short s;
	unsigned short t;

	if (sz < 12)
		acl_msg_fatal("%s: sz(%d) < 12", myname, (int) sz);

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
	if (off != 12)
		acl_msg_fatal("%s: off(%d) != 12", myname, off);
	return off;
}

/*
* rfc1035LabelPack()
* 
* Packs a label into a buffer.  The format of
* a label is one octet specifying the number of character
* bytes to follow.  Labels must be smaller than 64 octets.
* Returns number of octets packed.
*/
static int rfc1035LabelPack(char *buf, size_t sz, const char *label)
{
	const char *myname = "rfc1035LabelPack";
	int off = 0;
	size_t len = label ? strlen(label) : 0;

	if (label) {
		if (strchr(label, '.') != NULL)
			acl_msg_fatal("%s: '.' exist in label(%s)", myname, label);
	}

	if (len > RFC1035_MAXLABELSZ)
		len = RFC1035_MAXLABELSZ;
	if (sz < len + 1)
		acl_msg_fatal("%s: sz(%d) < len(%d) + 1", myname, (int) sz, (int) len);
	*(buf + off) = (char) len;
	off++;
	memcpy(buf + off, label, len);
	off += (int) len;
	return off;
}

/*
* rfc1035NamePack()
* 
* Packs a name into a buffer.  Names are packed as a
* sequence of labels, terminated with NULL label.
* Note message compression is not supported here.
* Returns number of octets packed.
*/
static int rfc1035NamePack(char *buf, size_t sz, const char *name)
{
	const char *myname = "rfc1035NamePack";
	int off = 0;
	char *copy, *ptr;
	char *t;

	copy = acl_mystrdup(name);
	/*
	* NOTE: use of strtok here makes names like foo....com valid.
	*/
	ptr = copy;
	for (t = acl_mystrtok(&ptr, "."); t; t = acl_mystrtok(&ptr, "."))
		off += rfc1035LabelPack(buf + off, sz - off, t);
	acl_myfree(copy);
	off += rfc1035LabelPack(buf + off, sz - off, NULL);
	if (off > (int) sz)
		acl_msg_fatal("%s: off(%d) > sz(%d)", myname, off, (int) sz);
	return off;
}

/*
* rfc1035QuestionPack()
* 
* Packs a QUESTION section of a message.
* Returns number of octets packed.
*/
static int rfc1035QuestionPack(char *buf, size_t sz, const char *name,
					unsigned short type, unsigned short tclass)
{
	const char *myname = "rfc1035QuestionPack";
	int off = 0;
	unsigned short s;

	off += rfc1035NamePack(buf + off, sz - off, name);
	s = htons(type);
	memcpy(buf + off, &s, sizeof(s));
	off += sizeof(s);
	s = htons(tclass);
	memcpy(buf + off, &s, sizeof(s));
	off += sizeof(s);
	if (off > (int) sz)
		acl_msg_fatal("%s: off(%d) > sz(%d)", myname, off, (int) sz);

	return off;
}

/*
* rfc1035HeaderUnpack()
* 
* Unpacks a RFC1035 message header buffer into the header fields
* of the rfc1035_message structure.
*
* Updates the buffer offset, which is the same as number of
* octects unpacked since the header starts at offset 0.
*
* Returns 0 (success) or 1 (error)
*/
static int rfc1035HeaderUnpack(const char *buf, size_t sz, int *off,
				rfc1035_message * h)
{
	const char *myname = "rfc1035HeaderUnpack";
	unsigned short s;
	unsigned short t;

	if (*off != 0)
		acl_msg_fatal("%s: *off(%d) != 0", myname, *off);

	/*
	* The header is 12 octets.  This is a bogus message if the size
	* is less than that.
	*/
	if (sz < 12)
		return 1;
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
	/*
	* We might want to check that the reserved 'Z' bits (6-4) are
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

	if (*off != 12)
		acl_msg_fatal("%s: *off(%d) != 12", myname, *off);
	return 0;
}

/*
* rfc1035NameUnpack()
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
static int rfc1035NameUnpack(const char *buf, size_t sz, int *off,
				 unsigned short *rdlength, char *name, size_t ns, int rdepth)
{
	const char *myname = "rfc1035NameUnpack";
	int no = 0;
	unsigned char c;
	size_t len;

	if (ns <= 0)
		acl_msg_fatal("%s: ns(%d) <= 0", myname, (int) ns);
	do {
		if (*off >= (int) sz)
			acl_msg_fatal("%s: *off(%d) >= sz(%d)", myname, *off, (int) sz);
		c = *(buf + (*off));
		if (c > 191) {
			/* blasted compression */
			unsigned short s;
			int ptr;
			if (rdepth > 64)	/* infinite pointer loop */
				return 1;
			memcpy(&s, buf + (*off), sizeof(s));
			s = ntohs(s);
			(*off) += sizeof(s);
			/* Sanity check */
			if ((*off) >= (int) sz)
				return 1;
			ptr = s & 0x3FFF;
			/* Make sure the pointer is inside this message */
			if (ptr >= (int) sz)
				return 1;
			return rfc1035NameUnpack(buf, sz, &ptr, rdlength, name + no,
						ns - no, rdepth + 1);
		} else if (c > RFC1035_MAXLABELSZ) {
			/*
			* "(The 10 and 01 combinations are reserved for future use.)"
			*/
			return 1;
		} else {
			(*off)++;
			len = (size_t) c;
			if (len == 0)
				break;
			if (len > (ns - no - 1))	/* label won't fit */
				return 1;
			if ((*off) + len >= sz)	/* message is too short */
				return 1;
			memcpy(name + no, buf + (*off), len);
			(*off) += (int) len;
			no += (int) len;
			*(name + (no++)) = '.';
			if (rdlength)
				*rdlength += (unsigned short) len + 1;
		}
	} while (c > 0 && no < (int) ns);
	if (no)
		*(name + no - 1) = '\0';
	else
		*name = '\0';
	/* make sure we didn't allow someone to overflow the name buffer */
	if (no > (int) ns)
		acl_msg_fatal("%s: no(%d) > ns(%d)", myname, no, (int) ns);
	return 0;
}

/*
* rfc1035RRUnpack()
* 
* Unpacks a RFC1035 Resource Record into 'RR' from a message buffer.
* The caller must free RR->rdata!
*
* Updates the new message buffer offset.
*
* Returns 0 (success) or 1 (error)
*/
static int rfc1035RRUnpack(const char *buf, size_t sz, int *off, rfc1035_rr * RR)
{
	const char *myname = "rfc1035RRUnpack";
	unsigned short s;
	unsigned int i;
	unsigned short rdlength;
	int rdata_off;

	if (rfc1035NameUnpack(buf, sz, off, NULL, RR->name, RFC1035_MAXHOSTNAMESZ, 0)) {
		RFC1035_UNPACK_DEBUG;
		memset(RR, '\0', sizeof(*RR));
		return 1;
	}
	/*
	* Make sure the remaining message has enough octets for the
	* rest of the RR fields.
	*/
	if ((*off) + 10 > (int) sz) {
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
	if ((*off) + rdlength > (int) sz) {
		/*
		* We got a truncated packet.  'dnscache' truncates UDP
		* replies at 512 octets, as per RFC 1035.
		*/
		RFC1035_UNPACK_DEBUG;
		memset(RR, '\0', sizeof(*RR));
		return 1;
	}
	RR->rdlength = rdlength;
	switch (RR->type) {
	case RFC1035_TYPE_PTR:
		RR->rdata = (char*) acl_mymalloc(RFC1035_MAXHOSTNAMESZ);
		rdata_off = *off;
		RR->rdlength = 0;	/* Filled in by rfc1035NameUnpack */
		if (rfc1035NameUnpack(buf, sz, &rdata_off, &RR->rdlength,
				RR->rdata, RFC1035_MAXHOSTNAMESZ, 0))
			return 1;
		if (rdata_off > ((*off) + rdlength)) {
			/*
			* This probably doesn't happen for valid packets, but
			* I want to make sure that NameUnpack doesn't go beyond
			* the RDATA area.
			*/
			RFC1035_UNPACK_DEBUG;
			acl_myfree(RR->rdata);
			memset(RR, '\0', sizeof(*RR));
			return 1;
		}
		break;
	case RFC1035_TYPE_A:
	default:
		RR->rdata = (char*) acl_mymalloc(rdlength);
		memcpy(RR->rdata, buf + (*off), rdlength);
		break;
	}
	(*off) += rdlength;
	if (*off > (int) sz)
		acl_msg_fatal("%s: *off(%d) > sz(%d)", myname, *off, (int) sz);
	return 0;
}

const char *rfc1035Strerror(int errnum)
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
		{ rfc1035_unpack_error, "The DNS reply message is corrupt or could "
			"not be safely parsed." },
		{ -1, NULL },
	};
	const char *unknown = "Unknown Error";
	int   i;

	for (i = 0; errmsg[i].msg != NULL; i++) {
		if (errmsg[i].errnum == -errnum)
			return (errmsg[i].msg);
	}

	return (unknown);
}

static void rfc1035SetErrno(int n)
{
	switch (rfc1035_errno = n) {
	case 0:
		rfc1035_error_message = "No error condition";
		break;
	case 1:
		rfc1035_error_message = "Format Error: The name server was "
			"unable to interpret the query.";
		break;
	case 2:
		rfc1035_error_message = "Server Failure: The name server was "
			"unable to process this query.";
		break;
	case 3:
		rfc1035_error_message = "Name Error: The domain name does "
			"not exist.";
		break;
	case 4:
		rfc1035_error_message = "Not Implemented: The name server does "
			"not support the requested kind of query.";
		break;
	case 5:
		rfc1035_error_message = "Refused: The name server refuses to "
			"perform the specified operation.";
		break;
	case rfc1035_unpack_error:
		rfc1035_error_message = "The DNS reply message is corrupt or could "
			"not be safely parsed.";
		break;
	default:
		rfc1035_error_message = "Unknown Error";
		break;
	}
}

static void rfc1035RRDestroy(rfc1035_rr * rr, int n)
{
	const char *myname = "rfc1035RRDestroy";

	if (rr == NULL)
		return;
	if (n <= 0)
		acl_msg_fatal("%s: n(%d) <= 0", myname, n);
	while (n--) {
		if (rr[n].rdata)
			acl_myfree(rr[n].rdata);
	}
	acl_myfree(rr);
}

/*
* rfc1035QueryUnpack()
* 
* Unpacks a RFC1035 Query Record into 'query' from a message buffer.
*
* Updates the new message buffer offset.
*
* Returns 0 (success) or 1 (error)
*/
static int rfc1035QueryUnpack(const char *buf, size_t sz, int *off,
				rfc1035_query * query)
{
	unsigned short s;

	if (rfc1035NameUnpack(buf, sz, off, NULL, query->name, RFC1035_MAXHOSTNAMESZ, 0)) {
		RFC1035_UNPACK_DEBUG;
		memset(query, '\0', sizeof(*query));
		return 1;
	}
	if (*off + 4 > (int) sz) {
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

void rfc1035MessageDestroy(rfc1035_message * msg)
{
	if (!msg)
		return;
	if (msg->query)
		acl_myfree(msg->query);
	if (msg->answer)
		rfc1035RRDestroy(msg->answer, msg->ancount);
	acl_myfree(msg);
}

/*
* rfc1035QueryCompare()
* 
* Compares two rfc1035_query entries
*
* Returns 0 (equal) or !=0 (different)
*/
int rfc1035QueryCompare(const rfc1035_query * a, const rfc1035_query * b)
{
	size_t la, lb;

	if (a->qtype != b->qtype)
		return 1;
	if (a->qclass != b->qclass)
		return 1;
	la = strlen(a->name);
	lb = strlen(b->name);
	if (la != lb) {
		/* Trim root label(s) */
		while (la > 0 && a->name[la - 1] == '.')
			la--;
		while (lb > 0 && b->name[lb - 1] == '.')
			lb--;
	}
	if (la != lb)
		return 1;

	return strncasecmp(a->name, b->name, la);
}

/*
* rfc1035MessageUnpack()
*
* Takes the contents of a DNS reply and fills in an array
* of resource record structures.  The records array is allocated
* here, and should be freed by calling rfc1035RRDestroy().
*
* Returns number of records unpacked, zero if DNS reply indicates
* zero answers, or an error number < 0.
*/

int rfc1035MessageUnpack(const char *buf, size_t sz, rfc1035_message ** answer)
{
	int off = 0;
	int i;
	int nr = 0;
	rfc1035_message *msg;
	rfc1035_rr *recs;
	rfc1035_query *querys;

	msg = (rfc1035_message*) acl_mycalloc(1, sizeof(*msg));
	if (rfc1035HeaderUnpack(buf + off, sz - off, &off, msg)) {
		RFC1035_UNPACK_DEBUG;
		rfc1035SetErrno(rfc1035_unpack_error);
		acl_myfree(msg);
		return -rfc1035_unpack_error;
	}
	rfc1035_errno = 0;
	rfc1035_error_message = NULL;
	i = (int) msg->qdcount;
	if (i != 1) {
		/* This can not be an answer to our queries.. */
		RFC1035_UNPACK_DEBUG;
		rfc1035SetErrno(rfc1035_unpack_error);
		acl_myfree(msg);
		return -rfc1035_unpack_error;
	}
	querys = msg->query = (rfc1035_query*) acl_mycalloc((int) msg->qdcount, sizeof(*querys));
	for (i = 0; i < (int) msg->qdcount; i++) {
		if (rfc1035QueryUnpack(buf, sz, &off, &querys[i])) {
			RFC1035_UNPACK_DEBUG;
			rfc1035SetErrno(rfc1035_unpack_error);
			rfc1035MessageDestroy(msg);
			return -rfc1035_unpack_error;
		}
	}
	*answer = msg;
	if (msg->rcode) {
		RFC1035_UNPACK_DEBUG;
		rfc1035SetErrno((int) msg->rcode);
		return -((int) msg->rcode);
	}
	if (msg->ancount == 0)
		return 0;
	recs = msg->answer = (rfc1035_rr*) acl_mycalloc((int) msg->ancount, sizeof(*recs));
	for (i = 0; i < (int) msg->ancount; i++) {
		if (off >= (int) sz) {	/* corrupt packet */
			RFC1035_UNPACK_DEBUG;
			break;
		}
		if (rfc1035RRUnpack(buf, sz, &off, &recs[i])) {		/* corrupt RR */
			RFC1035_UNPACK_DEBUG;
			break;
		}
		nr++;
	}
	if (nr == 0) {
		/*
		* we expected to unpack some answers (ancount != 0), but
		* didn't actually get any.
		*/
		rfc1035MessageDestroy(msg);
		*answer = NULL;
		rfc1035SetErrno(rfc1035_unpack_error);
		return -rfc1035_unpack_error;
	}

	if (msg->nscount > 0) {
		//rfc1035NSUnpack(buf, sz, &off);
	}

	if (msg->arcount > 0) {
		//rfc1035ARUnpack(buf, sz, &off);
	}
	return nr;
}

/*
* rfc1035RRPack()
*
* Unpacks a RFC1035 Resource Record into 'RR' from a message buffer.
* The caller must free RR->rdata!
*
* Updates the new message buffer offset.
*
* Returns > 0 (success) or 0 (error)
*/
static int rfc1035RRPack(const rfc1035_rr *RR, char *buf, size_t sz)
{
	const char *myname = "rfc1035RRPack";
	unsigned short s;
	unsigned int i;
	int off = 0, off_saved;

	off = rfc1035NamePack(buf + off, sz, RR->name);   
	s = htons(RR->type);
	memcpy(buf + off, &s,sizeof(s));
	off += sizeof(s);

	s = htons(RR->tclass);
	memcpy(buf + off, &s ,sizeof(s));
	off += sizeof(s);
    
	i = htonl(RR->ttl);
	memcpy(buf + off, &i ,sizeof(i));
	off += sizeof(i);

	switch (RR->type) {
	case RFC1035_TYPE_PTR:
	case RFC1035_TYPE_NS:
		if (strlen(RR->rdata) > RFC1035_MAXHOSTNAMESZ)
			return (0);
		off_saved = off;
		off += sizeof(s);
		off += rfc1035NamePack(buf + off, sz, RR->rdata);
		s = off - off_saved - (unsigned short) sizeof(s);
		s = htons(s);
		memcpy(buf + off_saved, &s ,sizeof(s));
		break;
	default:
		s = htons(RR->rdlength);
		memcpy(buf + off, &s ,sizeof(s));
		off += sizeof(s);
		memcpy(buf + off, RR->rdata, RR->rdlength);
		off += RR->rdlength;
		break;
	}

	if ((unsigned) off > sz)
		acl_msg_fatal("%s: off(%d) > sz(%d)", myname, (int) off, (int) sz);

	return (off);
}

ssize_t rfc1035BuildAReply(const char *hostname, const ACL_ARGV *ip_argv,
	const char *dnsname, const char *dns_ip,
	unsigned short qid, char *buf, size_t sz)
{
	rfc1035_message h;
	rfc1035_rr rr;
	size_t offset = 0;
	unsigned int nip;
	int   i;

	memset(&h, '\0', sizeof(h));
	h.id = qid;
	h.qr = 1;
	h.opcode = 0;		/* QUERY */
	h.aa = 0;
	h.tc = 0;
	h.rd = 1;
	h.ra = 0;
	h.rcode = 0;
	h.qdcount = 1;
	h.ancount = ip_argv->argc;
	h.nscount = (dnsname && *dnsname) ? 1 : 0;
	h.arcount = (h.nscount && dns_ip && *dns_ip) ? 1 : 0;
	offset += rfc1035HeaderPack(buf + offset, sz - offset, &h);
	offset += rfc1035QuestionPack(buf + offset, sz - offset, hostname,
		RFC1035_TYPE_A, RFC1035_CLASS_IN);

	for (i = 0; i < ip_argv->argc; i++) {
		memset(&rr, 0, sizeof(rr));
		snprintf(rr.name, sizeof(rr.name), "%s", hostname);
		rr.type = RFC1035_TYPE_A;
		rr.tclass = RFC1035_CLASS_IN;
		rr.ttl = 5;
		rr.rdlength = 4;
		nip = inet_addr(ip_argv->argv[i]);
		rr.rdata = (char*) acl_mycalloc(1, rr.rdlength);
		memcpy(rr.rdata, &nip, rr.rdlength);
		offset += rfc1035RRPack(&rr, buf + offset, sz - offset);
		acl_myfree(rr.rdata);
	}

	if (h.nscount) {
		memset(&rr, 0, sizeof(rr));
		snprintf(rr.name, sizeof(rr.name), "%s", dnsname);
		rr.type = RFC1035_TYPE_NS;
		rr.tclass = RFC1035_CLASS_IN;
		rr.ttl = 5;
		rr.rdlength = (unsigned short) strlen(dnsname);
		rr.rdata = acl_mystrdup(dnsname);
		offset += rfc1035RRPack(&rr, buf + offset, sz - offset);
		acl_myfree(rr.rdata);
	}

	if (h.arcount) {
		memset(&rr, 0, sizeof(rr));
		snprintf(rr.name, sizeof(rr.name), "%s", dnsname);
		rr.type = RFC1035_TYPE_NS;
		rr.tclass = RFC1035_CLASS_IN;
		rr.ttl = 5;
		rr.rdlength = (unsigned short) strlen(dns_ip);
		rr.rdata = acl_mystrdup(dns_ip);
		offset += rfc1035RRPack(&rr, buf + offset, sz - offset);
		acl_myfree(rr.rdata);
	}

	return ((int) offset);
}
/*
* rfc1035BuildAQuery()
* 
* Builds a message buffer with a QUESTION to lookup A records
* for a hostname.  Caller must allocate 'buf' which should
* probably be at least 512 octets.  The 'szp' initially
* specifies the size of the buffer, on return it contains
* the size of the message (i.e. how much to write).
* Returns the size of the query
*/
ssize_t rfc1035BuildAQuery(const char *hostname, char *buf, size_t sz,
			unsigned short qid, rfc1035_query * query)
{
	const char *myname = "rfc1035BuildAQuery";
	rfc1035_message h;
	size_t offset = 0;

	memset(&h, '\0', sizeof(h));
	h.id = qid;
	h.qr = 0;
	h.rd = 1;
	h.opcode = 0;		/* QUERY */
	h.qdcount = (unsigned int) 1;
	offset += rfc1035HeaderPack(buf + offset, sz - offset, &h);
	offset += rfc1035QuestionPack(buf + offset, sz - offset, hostname,
					RFC1035_TYPE_A, RFC1035_CLASS_IN);
	if (query) {
		query->qtype = RFC1035_TYPE_A;
		query->qclass = RFC1035_CLASS_IN;
		ACL_SAFE_STRNCPY(query->name, hostname, sizeof(query->name));
	}

	if (offset > sz)
		acl_msg_fatal("%s: offset(%d) > sz(%d)", myname, (int) offset, (int) sz);
	return (ssize_t) offset;
}

/*
* rfc1035BuildPTRQuery()
* 
* Builds a message buffer with a QUESTION to lookup PTR records
* for an address.  Caller must allocate 'buf' which should
* probably be at least 512 octets.  The 'szp' initially
* specifies the size of the buffer, on return it contains
* the size of the message (i.e. how much to write).
* Returns the size of the query
*/
ssize_t rfc1035BuildPTRQuery(const struct in_addr addr, char *buf,
			size_t sz, unsigned short qid, rfc1035_query * query)
{
	const char *myname = "rfc1035BuildPTRQuery";
	rfc1035_message h;
	size_t offset = 0;
	static char rev[32];
	unsigned int i;

	memset(&h, '\0', sizeof(h));
	i = (unsigned int) ntohl(addr.s_addr);
	snprintf(rev, 32, "%u.%u.%u.%u.in-addr.arpa.",
		i & 255, (i >> 8) & 255, (i >> 16) & 255, (i >> 24) & 255);
	h.id = qid;
	h.qr = 0;
	h.rd = 1;
	h.opcode = 0;		/* QUERY */
	h.qdcount = (unsigned int) 1;
	offset += rfc1035HeaderPack(buf + offset, sz - offset, &h);
	offset += rfc1035QuestionPack(buf + offset, sz - offset, rev,
				RFC1035_TYPE_PTR, RFC1035_CLASS_IN);
	if (query) {
		query->qtype = RFC1035_TYPE_PTR;
		query->qclass = RFC1035_CLASS_IN;
		ACL_SAFE_STRNCPY(query->name, rev, sizeof(query->name));
	}
	if (offset > sz)
		acl_msg_fatal("%s: offset(%d) > sz(%d)", myname, (int) offset, (int) sz);
	return (ssize_t) offset;
}

/*
* We're going to retry a former query, but we
* just need a new ID for it.  Lucky for us ID
* is the first field in the message buffer.
*/
void rfc1035SetQueryID(char *buf, unsigned short qid)
{
	unsigned short s = htons(qid);
	memcpy(buf, &s, sizeof(s));
}
