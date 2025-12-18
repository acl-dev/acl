#ifndef __LIB_ICMP_TYPE_INCLUDE_H__
#define __LIB_ICMP_TYPE_INCLUDE_H__

#define ICMP_TYPE_ECHOREPLY	0
#define ICMP_TYPE_ECHO		8

#define ICMP_CODE_EXTRA		16

typedef struct ICMP_STREAM ICMP_STREAM;
typedef struct ICMP_CHAT ICMP_CHAT;
typedef struct ICMP_STAT ICMP_STAT;
typedef struct ICMP_HOST ICMP_HOST;
typedef struct ICMP_PKT ICMP_PKT;
typedef struct ICMP_PKT_STATUS ICMP_PKT_STATUS;

/* ICMP common communication process PING response status info for each host */
struct ICMP_STAT {
	double tmin;			/* Minimum time */
	double tmax;			/* Maximum time */
	double tsum;			/* Total time */
	double tave;			/* Average time */
	size_t nsent;			/* Number of packets sent */
	size_t nreceived;		/* Number of packets received */
	double loss;			/* Number of packets lost */
};

#define ICMP_MIN_PACKET		32
#define ICMP_MAX_PACKET		1024

/* ICMP: Status response for each PING packet sent */
struct ICMP_PKT_STATUS {
	size_t reply_len;		/* Response data length */
	char   from_ip[64];		/* Source address */

	double         rtt;		/* Round Trip Time (milliseconds) */
	unsigned short seq;		/* Sequence number (seq no) */
	unsigned char  ttl;		/* Time to live */
	unsigned int   gid;
	char          *data;
	size_t         dlen;
	char           status;
#define ICMP_STATUS_INIT		0
#define ICMP_STATUS_OK			1
#define ICMP_STATUS_UNREACH		(1<<1) 
#define ICMP_STATUS_TIMEOUT		(1<<2)

	ICMP_PKT      *pkt;
};

/* Target host information structure */
struct ICMP_HOST {
	ICMP_STAT icmp_stat;		/* ICMP communication status */
	char dest_ip[32];		/* Target host IP address */
	char domain[64];		/* Domain corresponding to target host IP */
	struct sockaddr_in dest;	/* Target host address when sending */
	struct sockaddr_in from;	/* Source host address when receiving */
	int   from_len;			/* Address length stored in from */
	int   delay;			/* Interval between PINGs (milliseconds) */
	int   timeout;			/* Timeout for probe response (milliseconds) */
	size_t dlen;			/* Size of each packet sent (bytes) */
	size_t nsent;			/* Number of packets sent to target host */

	ICMP_PKT **pkts;		/* Array of all packets */
	size_t npkt;			/* Number of packets sent to target host */
	size_t ipkt;			/* Record next packet index to send */

	ACL_RING   host_ring;		/* Linked to ICMP_CHAT->host_head list */
	ICMP_CHAT *chat;		/* Communication object for this host */
	char  enable_log;		/* Whether to report response info to log */

	/* Report packet response status */
	void (*stat_respond)(ICMP_PKT_STATUS*, void*);

	/* Called when packet response times out */
	void (*stat_timeout)(ICMP_PKT_STATUS*, void*);

	/* Called when host is unreachable */
	void (*stat_unreach)(ICMP_PKT_STATUS*, void*);

	/* Callback when all packets for host complete */
	void (*stat_finish)(ICMP_HOST*, void*);

	/* Private parameter address passed by application */
	void *arg;
};

#endif

