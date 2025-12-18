#ifndef __LIB_ICMP_INCLUDE_H__
#define __LIB_ICMP_INCLUDE_H__

/* #include "lib_acl.h" */
#include "lib_icmp_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ICMP_LIB
# ifndef ICMP_API
#  define ICMP_API
# endif
#elif defined(ICMP_DLL) /* || defined(_WINDLL) */
# if defined(ICMP_EXPORTS) || defined(protocol_EXPORTS)
#  ifndef ICMP_API
#   define ICMP_API __declspec(dllexport)
#  endif
# elif !defined(ICMP_API)
#  define ICMP_API __declspec(dllimport)
# endif
#elif !defined(ICMP_API)
# define ICMP_API
#endif

/* in icmp_chat.c */
/**
 * Create ICMP session object
 * @param aio {ACL_AIO*} If not NULL, communication will use async mode
 *  internally; otherwise uses sync mode
 * @param check_tid {int} Whether to check thread ID when response arrives
 * @return {ICMP_CHAT*} ICMP session object
 */
ICMP_API ICMP_CHAT *icmp_chat_create(ACL_AIO *aio, int check_tid);

/**
 * Free ICMP session object
 * @param chat {ICMP_CHAT*} ICMP session object
 */
ICMP_API void icmp_chat_free(ICMP_CHAT *chat);

/**
 * Start session for probing a target host
 * @param host {ICMP_HOST*} Object returned by icmp_host_new
 */
ICMP_API void icmp_chat(ICMP_HOST* host);

/**
 * Get number of hosts being probed in current ICMP session list
 * @param chat {ICMP_CHAT*} Session object
 * @return {int} Number of hosts being probed
 */
ICMP_API int icmp_chat_size(ICMP_CHAT *chat);

/**
 * Get number of completed probes in current ICMP session
 * @param chat {ICMP_CHAT*} Session object
 * @return {int} Number of completed probes
 */
ICMP_API int icmp_chat_count(ICMP_CHAT *chat);

/**
 * Check if all probe tasks in current ICMP session have completed
 * @param chat {ICMP_CHAT*} Session object
 * @return {int} != 0: indicates complete; 0: indicates incomplete
 */
ICMP_API int icmp_chat_finish(ICMP_CHAT *chat);

/**
 * Get current session sequence number value in ICMP session
 * @param chat {ICMP_CHAT*} Session object
 * @return {unsigned short} Session sequence number value
 */
ICMP_API unsigned short icmp_chat_seqno(ICMP_CHAT *chat);

/* in icmp_stat.c */
/**
 * Print current ICMP session status
 * @param chat {ICMP_CHAT*} Session object
 */
ICMP_API void icmp_stat(ICMP_CHAT *chat);

/**
 * Print ICMP session status for a specific host
 * @param host {ICMP_HOST*} Probe host object
 * @param show_flag {int} Whether to print to standard log file
 */
ICMP_API void icmp_stat_host(ICMP_HOST *host, int show_flag);

/* in icmp_host.c */
/**
 * Create a new probe host object
 * @param chat {ICMP_CHAT*} Session object
 * @param domain {const char*} Host identifier string, not NULL
 * @param ip {const char*} Host IP address, not NULL
 * @param npkt {size_t} Number of packets to send to this host
 * @param dlen {size_t} Length of each probe packet
 * @param delay {int} Interval between packets (milliseconds)
 * @param timeout {int} Response timeout for probe packets (milliseconds)
 * @return {ICMP_HOST*} Probe host object, NULL indicates error
 */
ICMP_API ICMP_HOST* icmp_host_new(ICMP_CHAT *chat, const char *domain,
	const char *ip, size_t npkt, size_t dlen, int delay, int timeout);

/**
 * Free a probe host object
 * @param host {ICMP_HOST*} Probe host object
 */
ICMP_API void icmp_host_free(ICMP_HOST *host);

/**
 * Set callback functions for probe host
 * @param host {ICMP_HOST*} Probe host object
 * @param arg {void*} One of the callback function parameters
 * @param stat_respond {void (*)(ICMP_PKT_STATUS*)} Callback when packet
 *  receives response
 * @param stat_timeout {void (*)(ICMP_PKT_STATUS*)} Callback when response
 *  times out
 * @param stat_unreach {void (*)(ICMP_PKT_STATUS*}} Callback when host
 *  unreachable
 * @param stat_finish {void (*)(ICMP_HOST*)} Callback when all probes
 *  complete for this host
 */
ICMP_API void icmp_host_set(ICMP_HOST *host, void *arg,
	void (*stat_respond)(ICMP_PKT_STATUS*, void*),
	void (*stat_timeout)(ICMP_PKT_STATUS*, void*),
	void (*stat_unreach)(ICMP_PKT_STATUS*, void*),
	void (*stat_finish)(ICMP_HOST*, void*));

/* in icmp_ping.c */
/**
 * Ping a host (internally defaults each probe packet to 64 bytes)
 * @param chat {ICMP_CHAT*} Session object
 * @param domain {const char*} Host identifier string, not NULL
 * @param ip {const char*} Host IP address, not NULL
 * @param npkt {size_t} Number of packets to send to this host
 * @param delay {int} Interval between packets (milliseconds)
 * @param timeout {int} Response timeout for probe packets (milliseconds)
 */
ICMP_API void icmp_ping_one(ICMP_CHAT *chat, const char *domain,
	const char *ip, size_t npkt, int delay, int timeout);

/*--------------------------------------------------------------------------*/

/**
 * low level interface for operating ICMP.
 */

ICMP_API ICMP_STREAM* icmp_stream_open(ACL_AIO *aio);
ICMP_API void icmp_stream_close(ICMP_STREAM* is);
ICMP_API ACL_VSTREAM *icmp_vstream(ICMP_STREAM *is);
ICMP_API void icmp_stream_from(ICMP_STREAM *is, struct sockaddr_in *addr);
ICMP_API void icmp_stream_dest(ICMP_STREAM *is, struct sockaddr_in *addr);
ICMP_API void icmp_stream_set_dest(ICMP_STREAM *is, struct sockaddr_in addr);

ICMP_API ICMP_HOST *icmp_host_alloc(ICMP_CHAT *chat, const char *domain,
		const char *ip);
ICMP_API void icmp_host_init(ICMP_HOST *host, unsigned char type,
		unsigned char code, size_t npkt, size_t dlen,
		int delay, int timeout);

ICMP_API ICMP_PKT *icmp_pkt_alloc(void);
ICMP_API void icmp_pkt_free(ICMP_PKT *ipkt);
ICMP_API void icmp_pkt_pack(ICMP_PKT *pkt, unsigned char type,
		unsigned char code, unsigned short id,
		const void *payload, size_t payload_len);
ICMP_API void icmp_pkt_build(ICMP_PKT *pkt, unsigned short seq);
ICMP_API void icmp_pkt_save_status(ICMP_PKT* to, const ICMP_PKT* from);
ICMP_API int  icmp_pkt_unpack(struct sockaddr_in from, const char *buf,
		int bytes, ICMP_PKT *pkt);
ICMP_API ICMP_PKT* icmp_pkt_check(const ICMP_HOST *host, const ICMP_PKT *pkt);

ICMP_API unsigned char  icmp_pkt_type(const ICMP_PKT *pkt);
ICMP_API unsigned char  icmp_pkt_code(const ICMP_PKT *pkt);
ICMP_API unsigned short icmp_pkt_cksum(const ICMP_PKT *pkt);
ICMP_API unsigned short icmp_pkt_id(const ICMP_PKT *pkt);
ICMP_API unsigned short icmp_pkt_seq(const ICMP_PKT *pkt);
ICMP_API unsigned int   icmp_pkt_gid(const ICMP_PKT *pkt);
ICMP_API const ICMP_PKT *icmp_pkt_peer(const ICMP_PKT *pkt);
ICMP_API const ICMP_PKT_STATUS *icmp_pkt_status(const ICMP_PKT *pkt);
ICMP_API size_t icmp_pkt_len(const ICMP_PKT *pkt);
ICMP_API size_t icmp_pkt_wlen(const ICMP_PKT *pkt);
ICMP_API size_t icmp_pkt_payload(const ICMP_PKT *pkt, char *buf, size_t size);

ICMP_API size_t icmp_pkt_set_extra(ICMP_PKT *pkt,
		const void *data, size_t len);
ICMP_API void icmp_pkt_set_type(ICMP_PKT *pkt, unsigned char type);
ICMP_API void icmp_pkt_set_code(ICMP_PKT *pkt, unsigned char code);
ICMP_API void icmp_pkt_set_cksum(ICMP_PKT *pkt, unsigned short cksum);
ICMP_API void icmp_pkt_set_id(ICMP_PKT *pkt, unsigned short id);
ICMP_API void icmp_pkt_set_seq(ICMP_PKT *pkt, unsigned short seq);
ICMP_API void icmp_pkt_set_data(ICMP_PKT *pkt, void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif

