/*++
 * NAME
 *	tls_mgr 3
 * SUMMARY
 *	tlsmgr client interface
 * SYNOPSIS
 *	#include <tls_mgr.h>
 *
 *	int	tls_mgr_seed(buf, len)
 *	ACL_VSTRING	*buf;
 *	int	len;
 *
 *	int	tls_mgr_policy(cache_type, cachable)
 *	const char *cache_type;
 *	int	*cachable;
 *
 *	int	tls_mgr_update(cache_type, cache_id, buf, len)
 *	const char *cache_type;
 *	const char *cache_id;
 *	const char *buf;
 *	ssize_t	len;
 *
 *	int	tls_mgr_lookup(cache_type, cache_id, buf)
 *	const char *cache_type;
 *	const char *cache_id;
 *	ACL_VSTRING	*buf;
 *
 *	int	tls_mgr_delete(cache_type, cache_id)
 *	const char *cache_type;
 *	const char *cache_id;
 * DESCRIPTION
 *	These routines communicate with the tlsmgr(8) server for
 *	entropy and session cache management. Since these are
 *	non-critical services, requests are allowed to fail without
 *	disrupting Postfix.
 *
 *	tls_mgr_seed() requests entropy from the tlsmgr(8)
 *	Pseudo Random Number Generator (PRNG) pool.
 *
 *	tls_mgr_policy() requests the session caching policy.
 *
 *	tls_mgr_lookup() loads the specified session from
 *	the specified session cache.
 *
 *	tls_mgr_update() saves the specified session to
 *	the specified session cache.
 *
 *	tls_mgr_delete() removes specified session from
 *	the specified session cache.
 *
 *	Arguments
 * .IP cache_type
 *	One of TLS_MGR_SCACHE_SMTPD, TLS_MGR_SCACHE_SMTP
 * .IP cachable
 *	Pointer to int, set non-zero if the requested cache_type
 *	is enabled.
 * .IP cache_id
 *	The session cache lookup key.
 * .IP buf
 *	The result or input buffer.
 * .IP len
 *	The length of the input buffer, or the amount of data requested.
 * DIAGNOSTICS
 *	All client functions return one of the following status codes:
 * .IP TLS_MGR_STAT_OK
 *      The request completed, and the requested operation was
 *	successful (for example, the requested session was found,
 *	or the specified session was saved or removed).
 * .IP TLS_MGR_STAT_ERR
 *      The request completed, but the requested operation failed
 *	(for example, the requested object was not found or the
 *	specified session was not saved or removed).
 * .IP TLS_MGR_STAT_FAIL
 *      The request could not complete (the client could not
 *	communicate with the tlsmgr(8) server).
 * SEE ALSO
 *	tlsmgr(8) TLS session and PRNG management
 * LICENSE
 * .ad
 * .fi
 *	The Secure Mailer license must be distributed with this software.
 * AUTHOR(S)
 *	Wietse Venema
 *	IBM T.J. Watson Research
 *	P.O. Box 704
 *	Yorktown Heights, NY 10598, USA
 *--*/

#include "StdAfx.h"

#ifdef USE_TLS

#include <openssl/rand.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

#include "../attr/attr.h"
#include "../attr/attr_clnt.h"

/* Global library. */

#include "tls_params.h"
#include "tls_mgr.h"

/* Application-specific. */

static __thread ATTR_CLNT *tls_mgr = 0;

static void free_event(void *ctx)
{
    ACL_EVENT *eventp = (ACL_EVENT*) ctx;

    acl_event_free(eventp);
}

/* tls_mgr_open - create client handle */

void tls_mgr_open(ACL_EVENT *eventp)
{

    /*
     * Sanity check.
     */
    if (tls_mgr != 0) {
	if (acl_msg_verbose)
	    acl_msg_info("tls_mgr_open: multiple initialization");
	return;
    }

    if (eventp == NULL) {
	eventp = acl_event_new_select(1, 0);
	acl_pthread_atexit_add(eventp, free_event);
    }

    /*
     * Use whatever IPC is preferred for internal use: UNIX-domain sockets or
     * Solaris streams.
     */
#ifndef VAR_TLS_MGR_SERVICE
    tls_mgr = attr_clnt_create(eventp, "local:" TLS_MGR_CLASS "/" TLS_MGR_SERVICE,
			       var_ipc_timeout, var_ipc_idle_limit,
			       var_ipc_ttl_limit);
    if (tls_mgr == 0) {
	    acl_msg_warn("attr_clnt_create error, service: local:%s",
		TLS_MGR_CLASS "/" TLS_MGR_SERVICE);
	    return;
    }
#else
    tls_mgr = attr_clnt_create(eventp, var_tlsmgr_service, var_ipc_timeout,
			       var_ipc_idle_limit, var_ipc_ttl_limit);
    if (tls_mgr == 0) {
	    acl_msg_warn("attr_clnt_create error, service: %s", var_tlsmgr_service);
	    return;
    }
#endif

    acl_pthread_atexit_add(tls_mgr, (void (*)(void*)) attr_clnt_free);
    attr_clnt_control(tls_mgr,
		      ATTR_CLNT_CTL_PROTO, attr_vprint, attr_vscan,
		      ATTR_CLNT_CTL_END);
}

/* tls_mgr_seed - request PRNG seed */

int     tls_mgr_seed(ACL_VSTRING *buf, int len)
{
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0) {
	acl_msg_warn("tls_mgr_seed: call tls_mgr_open first");
	return TLS_MGR_STAT_FAIL;
    }

    /*
     * Request seed.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request attributes */
			  ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_SEED,
			  ATTR_TYPE_INT, TLS_MGR_ATTR_SIZE, len,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SEED, buf,
			  ATTR_TYPE_END) != 2)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_policy - request caching policy */

int     tls_mgr_policy(const char *cache_type, int *cachable)
{
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0) {
	if (acl_msg_verbose)
	    acl_msg_info("tls_mgr_policy: call tls_mgr_open first");
	return TLS_MGR_STAT_FAIL;
    }

    /*
     * Request policy.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request attributes */
			  ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_POLICY,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_INT, TLS_MGR_ATTR_CACHABLE, cachable,
			  ATTR_TYPE_END) != 2)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_lookup - request cached session */

int     tls_mgr_lookup(const char *cache_type, const char *cache_id, ACL_VSTRING *buf)
{
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0) {
	acl_msg_warn("tls_mgr_lookup: call tls_mgr_open first");
	return TLS_MGR_STAT_FAIL;
    }

    /*
     * Send the request and receive the reply.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request */
			  ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_LOOKUP,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SESSION, buf,
			  ATTR_TYPE_END) != 2)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_update - save session to cache */

int  tls_mgr_update(const char *cache_type, const char *cache_id,
	const char *buf, ssize_t len)
{
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0) {
	acl_msg_warn("tls_mgr_update: call tls_mgr_open first");
	return TLS_MGR_STAT_FAIL;
    }

    /*
     * Send the request and receive the reply.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request */
			  ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_UPDATE,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SESSION, len, buf,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_END) != 1)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

/* tls_mgr_delete - remove cached session */

int     tls_mgr_delete(const char *cache_type, const char *cache_id)
{
    int     status;

    /*
     * Create the tlsmgr client handle.
     */
    if (tls_mgr == 0) {
	acl_msg_warn("tls_mgr_delete: call tls_mgr_open first");
	return TLS_MGR_STAT_FAIL;
    }

    /*
     * Send the request and receive the reply.
     */
    if (attr_clnt_request(tls_mgr,
			  ATTR_FLAG_NONE,	/* Request */
			  ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, TLS_MGR_REQ_DELETE,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply */
			  ATTR_TYPE_INT, TLS_MGR_ATTR_STATUS, &status,
			  ATTR_TYPE_END) != 1)
	status = TLS_MGR_STAT_FAIL;
    return (status);
}

#ifdef TEST

/* System library. */

#include <stdlib.h>

/* Utility library. */

#include <argv.h>
#include <hex_code.h>

/* Global library. */

#include <config.h>

/* Application-specific. */

#define STR(x) acl_vstring_str(x)
#define LEN(x) ACL_VSTRING_LEN(x)

int     main(int unused_ac, char **av)
{
    ACL_VSTRING *inbuf = acl_vstring_alloc(10);
    int     status;
    ARGV   *argv = 0;
    ACL_EVENT *eventp = acl_event_new_select(1, 0);

    acl_msg_verbose = 3;

    mail_conf_read();
    acl_msg_info("using config files in %s", var_config_dir);

    if (chdir(var_queue_dir) < 0)
	acl_msg_fatal("chdir %s: %s", var_queue_dir, acl_last_serror());
    tls_mgr_open(eventp);

    while (acl_vstring_fgets_nonl(inbuf, ACL_VSTREAM_IN)) {
	argv = argv_split(STR(inbuf), " \t\r\n");
	if (argv->argc == 0) {
	    argv_free(argv);
	    continue;
	}

#define COMMAND(argv, str, len) \
    (strcasecmp(argv->argv[0], str) == 0 && argv->argc == len)

	if (COMMAND(argv, "policy", 2)) {
	    int     cachable;

	    status = tls_mgr_policy(argv->argv[1], &cachable);
	    acl_vstream_printf("status=%d cachable=%d\n", status, cachable);
	} else if (COMMAND(argv, "seed", 2)) {
	    ACL_VSTRING *buf = acl_vstring_alloc(10);
	    ACL_VSTRING *hex = acl_vstring_alloc(10);
	    int     len = atoi(argv->argv[1]);

	    status = tls_mgr_seed(buf, len);
	    hex_encode(hex, STR(buf), LEN(buf));
	    acl_vstream_printf("status=%d seed=%s\n", status, STR(hex));
	    acl_vstring_free(hex);
	    acl_vstring_free(buf);
	} else if (COMMAND(argv, "lookup", 3)) {
	    ACL_VSTRING *buf = acl_vstring_alloc(10);

	    status = tls_mgr_lookup(argv->argv[1], argv->argv[2], buf);
	    acl_vstream_printf("status=%d session=%.*s\n",
			   status, LEN(buf), STR(buf));
	    acl_vstring_free(buf);
	} else if (COMMAND(argv, "update", 4)) {
	    status = tls_mgr_update(argv->argv[1], argv->argv[2],
				    argv->argv[3], strlen(argv->argv[3]));
	    acl_vstream_printf("status=%d\n", status);
	} else if (COMMAND(argv, "delete", 3)) {
	    status = tls_mgr_delete(argv->argv[1], argv->argv[2]);
	    acl_vstream_printf("status=%d\n", status);
	} else {
	    acl_vstream_printf("usage:\n"
			   "seed byte_count\n"
			   "policy smtpd|smtp|lmtp\n"
			   "lookup smtpd|smtp|lmtp cache_id\n"
			   "update smtpd|smtp|lmtp cache_id session\n"
			   "delete smtpd|smtp|lmtp cache_id\n");
	}
	acl_vstream_fflush(ACL_VSTREAM_OUT);
	argv_free(argv);
    }

    acl_vstring_free(inbuf);
    acl_event_free(eventp);
    return (0);
}

#endif					/* TEST */

#endif					/* USE_TLS */
