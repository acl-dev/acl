#include "StdAfx.h"
#include <sys/stat.h>
#include <stdlib.h>
#ifdef	ACL_UNIX
# include <unistd.h>
# include <sys/time.h>			/* gettimeofday, not POSIX */
#elif defined(WIN32)
# include <process.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

/* OpenSSL library. */

#ifdef USE_TLS
#include <openssl/rand.h>		/* For the PRNG */
#endif

#include "../attr/attr.h"

#ifndef UCHAR_MAX
#define UCHAR_MAX 0xff
#endif

/* TLS library. */

#ifdef USE_TLS
#include "tls.h"			/* TLS_MGR_SCACHE_<type> */
#include "tls_private.h"
#include "tls_mgr.h"
#include "tls_prng.h"
#include "tls_scache.h"
#include "tls_params.h"

/* Application-specific. */

 /*
  * Tunables.
  */
char   *var_tls_rand_source;
int     var_tls_rand_bytes;
int     var_tls_reseed_period;
int     var_tls_prng_exch_period;
int     var_server_tls_loglevel;
char   *var_server_tls_scache_db;
int     var_server_tls_scache_timeout;
int     var_client_tls_loglevel;
char   *var_client_tls_scache_db;
int     var_client_tls_scache_timeout;
char   *var_tls_rand_exch_name;
int     var_tlsmgr_stand_alone = 0;

 /*
  * Bound the time that we are willing to wait for an I/O operation. This
  * produces better error messages than waiting until the watchdog timer
  * kills the process.
  */
#define TLS_MGR_TIMEOUT	10

 /*
  * State for updating the PRNG exchange file.
  */
static TLS_PRNG_SRC *rand_exch;

 /*
  * State for seeding the internal PRNG from external source.
  */
static TLS_PRNG_SRC *rand_source_dev;
static TLS_PRNG_SRC *rand_source_egd;
static TLS_PRNG_SRC *rand_source_file;

 /*
  * The external entropy source type is encoded in the source name. The
  * obvious alternative is to have separate configuration parameters per
  * source type, so that one process can query multiple external sources.
  */
#define DEV_PREF "dev:"
#define DEV_PREF_LEN (sizeof((DEV_PREF)) - 1)
#define DEV_PATH(dev) ((dev) + EGD_PREF_LEN)

#define EGD_PREF "egd:"
#define EGD_PREF_LEN (sizeof((EGD_PREF)) - 1)
#define EGD_PATH(egd) ((egd) + EGD_PREF_LEN)

 /*
  * State for TLS session caches.
  */
typedef struct {
    char   *cache_label;		/* cache short-hand name */
    TLS_SCACHE *cache_info;		/* cache handle */
    int     cache_active;		/* cache status */
    char  **cache_db;			/* main.cf parameter value */
    int    *cache_loglevel;		/* main.cf parameter value */
    int    *cache_timeout;		/* main.cf parameter value */
} TLSMGR_SCACHE;

TLSMGR_SCACHE cache_table[] = {
    { TLS_MGR_SCACHE_SERVER, 0, 0, &var_server_tls_scache_db,
	&var_server_tls_loglevel, &var_server_tls_scache_timeout },
    { TLS_MGR_SCACHE_CLIENT, 0, 0, &var_client_tls_scache_db,
	&var_client_tls_loglevel, &var_client_tls_scache_timeout },
    { 0, 0, 0, 0, 0, 0 },
};

static ACL_EVENT *__eventp;
 /*
  * SLMs.
  */
#define STR(x)		acl_vstring_str(x)
#define LEN(x)		ACL_VSTRING_LEN(x)
#define STREQ(x, y)	(strcmp((x), (y)) == 0)

/* tlsmgr_prng_exch_event - update PRNG exchange file */

static void tlsmgr_prng_exch_event(int event_type acl_unused,
	ACL_EVENT *event acl_unused, void *dummy)
{
    const char *myname = "tlsmgr_prng_exch_event";
    unsigned char randbyte;
    int     next_period;
    struct acl_stat st;

    if (acl_msg_verbose)
	acl_msg_info("%s: update PRNG exchange file", myname);

    /*
     * Sanity check. If the PRNG exchange file was removed, there is no point
     * updating it further. Restart the process and update the new file.
     */
    if (acl_fstat(rand_exch->fd.file, &st) < 0)
	acl_msg_fatal("%s: cannot fstat() the PRNG exchange file: %s", myname, acl_last_serror());
    if (st.st_nlink == 0) {
	acl_msg_warn("%s: PRNG exchange file was removed -- exiting to reopen", myname);
	sleep(1);
	exit(0);
    }
    tls_prng_exch_update(rand_exch);

    /*
     * Make prediction difficult for outsiders and calculate the time for the
     * next execution randomly.
     */
    RAND_bytes(&randbyte, 1);
    next_period = (var_tls_prng_exch_period * randbyte) / UCHAR_MAX;
    acl_event_request_timer(__eventp, tlsmgr_prng_exch_event, dummy, next_period, 0);
}

/* tlsmgr_reseed_event - re-seed the internal PRNG pool */

static void tlsmgr_reseed_event(int event_type acl_unused,
	ACL_EVENT *event acl_unused, void *dummy)
{
    const char *myname = "tlsmgr_reseed_event";
    int     next_period;
    unsigned char randbyte;
    int     must_exit = 0;

    /*
     * Reseed the internal PRNG from external source. Errors are recoverable.
     * We simply restart and reconnect without making a fuss. This is OK
     * because we do require that exchange file updates succeed. The exchange
     * file is the only entropy source that really matters in the long term.
     * 
     * If the administrator specifies an external randomness source that we
     * could not open upon start-up, restart to see if we can open it now
     * (and log a nagging warning if we can't).
     */
    if (*var_tls_rand_source) {

	/*
	 * Source is a random device.
	 */
	if (rand_source_dev) {
	    if (tls_prng_dev_read(rand_source_dev, var_tls_rand_bytes) <= 0) {
		acl_msg_info("%s: cannot read from entropy device %s: %s -- "
			 "exiting to reopen", myname, DEV_PATH(var_tls_rand_source),
			 acl_last_serror());
		must_exit = 1;
	    }
	}

	/*
	 * Source is an EGD compatible socket.
	 */
	else if (rand_source_egd) {
	    if (tls_prng_egd_read(rand_source_egd, var_tls_rand_bytes) <= 0) {
		acl_msg_info("%s: lost connection to EGD server %s -- "
		     "exiting to reconnect", myname, EGD_PATH(var_tls_rand_source));
		must_exit = 1;
	    }
	}

	/*
	 * Source is a regular file. Read the content once and close the
	 * file.
	 */
	else if (rand_source_file) {
	    if (tls_prng_file_read(rand_source_file, var_tls_rand_bytes) <= 0)
		acl_msg_warn("%s: cannot read from entropy file %s: %s",
			 myname, var_tls_rand_source, acl_last_serror());
	    tls_prng_file_close(rand_source_file);
	    rand_source_file = 0;
	    var_tls_rand_source[0] = 0;
	}

	/*
	 * Could not open the external source upon start-up. See if we can
	 * open it this time. Save PRNG state before we exit.
	 */
	else {
	    acl_msg_info("exiting to reopen external entropy source %s",
		     var_tls_rand_source);
	    must_exit = 1;
	}
    }

    /*
     * Save PRNG state in case we must exit.
     */
    if (must_exit) {
	if (rand_exch)
	    tls_prng_exch_update(rand_exch);
	sleep(1);
	acl_msg_info("exit now");
	exit(0);
    }

    /*
     * Make prediction difficult for outsiders and calculate the time for the
     * next execution randomly.
     */
    RAND_bytes(&randbyte, 1);
    next_period = (var_tls_reseed_period * randbyte) / UCHAR_MAX;
    acl_event_request_timer(__eventp, tlsmgr_reseed_event, dummy, next_period, 0);
}

/* tlsmgr_cache_run_event - start TLS session cache scan */

static void tlsmgr_cache_run_event(int event_type acl_unused,
	ACL_EVENT *event acl_unused, void *ctx)
{
    const char *myname = "tlsmgr_cache_run_event";
    TLSMGR_SCACHE *cache = (TLSMGR_SCACHE *) ctx;

    /*
     * This routine runs when it is time for another TLS session cache scan.
     * Make sure this routine gets called again in the future.
     * 
     * Don't start a new scan when the timer goes off while cache cleanup is
     * still in progress.
     */
    if (cache->cache_info->verbose)
	acl_msg_info("%s: start TLS %s session cache cleanup",
		 myname, cache->cache_label);

    if (cache->cache_active == 0)
	cache->cache_active =
	    tls_scache_sequence(cache->cache_info, DICT_SEQ_FUN_FIRST,
				TLS_SCACHE_SEQUENCE_NOTHING);

    acl_event_request_timer(__eventp, tlsmgr_cache_run_event, (char *) cache,
			cache->cache_info->timeout, 0);
}

/* tlsmgr_loop - TLS manager main loop */

static int tlsmgr_loop(char *unused_name acl_unused, char **unused_argv acl_unused)
{
    struct timeval tv;
    int     active = 0;
    TLSMGR_SCACHE *ent;

    /*
     * Update the PRNG pool with the time of day. We do it here after every
     * event (including internal timer events and external client request
     * events), instead of doing it in individual event call-back routines.
     */
    gettimeofday(&tv, 0);
    RAND_seed(&tv, sizeof(struct timeval));

    /*
     * This routine runs as part of the event handling loop, after the event
     * manager has delivered a timer or I/O event, or after it has waited for
     * a specified amount of time. The result value of tlsmgr_loop()
     * specifies how long the event manager should wait for the next event.
     * 
     * We use this loop to interleave TLS session cache cleanup with other
     * activity. Interleaved processing is needed when we use a client-server
     * protocol for entropy and session state exchange with client(8) and
     * server(8) processes.
     */
#define DONT_WAIT	0
#define WAIT_FOR_EVENT	(-1)

    for (ent = cache_table; ent->cache_label; ++ent) {
	if (ent->cache_info && ent->cache_active)
	    active |= ent->cache_active =
		tls_scache_sequence(ent->cache_info, DICT_SEQ_FUN_NEXT,
				    TLS_SCACHE_SEQUENCE_NOTHING);
    }

    return (active ? DONT_WAIT : WAIT_FOR_EVENT);
}

/* tlsmgr_request_receive - receive request */

static int tlsmgr_request_receive(ACL_VSTREAM *client_stream, ACL_VSTRING *request)
{
    const char *myname = "tlsmgr_request_receive";
    int     count;

    /*
     * Kluge: choose the protocol depending on the request size.
     */
    if (acl_read_wait(ACL_VSTREAM_SOCK(client_stream), var_ipc_timeout) < 0) {
	acl_msg_warn("%s: timeout while waiting for data from %s",
		 myname, ACL_VSTREAM_PATH(client_stream));
	return (-1);
    }
    if ((count = acl_peekfd(ACL_VSTREAM_SOCK(client_stream))) < 0) {
	acl_msg_warn("%s: cannot examine read buffer of %s: %s",
		 myname, ACL_VSTREAM_PATH(client_stream), acl_last_serror());
	return (-1);
    }

    /*
     * Short request: master trigger. Use the string+null protocol.
     */
    if (count <= 2) {
	if (acl_vstring_gets_null(request, client_stream) == ACL_VSTREAM_EOF) {
	    acl_msg_warn("%s: end-of-input while reading request from %s: %s",
		     myname, ACL_VSTREAM_PATH(client_stream), acl_last_serror());
	    return (-1);
	}
    }

    /*
     * Long request: real tlsmgr client. Use the attribute list protocol.
     */
    else {
	if (attr_scan(client_stream,
		      ATTR_FLAG_MORE | ATTR_FLAG_STRICT,
		      ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, request,
		      ATTR_TYPE_END) != 1) {
	    return (-1);
	}
    }
    return (0);
}

/* tlsmgr_service - respond to external request */

static void tlsmgr_service(ACL_VSTREAM *client_stream,
	char *unused_service acl_unused, char **argv acl_unused)
{
    const char *myname = "tlsmgr_service";
    static ACL_VSTRING *request = 0;
    static ACL_VSTRING *cache_type = 0;
    static ACL_VSTRING *cache_id = 0;
    static ACL_VSTRING *buffer = 0;
    int     len;
    static char wakeup[] = {            /* master wakeup request */
	TRIGGER_REQ_WAKEUP,
	0,
    };
    TLSMGR_SCACHE *ent;
    int     status = TLS_MGR_STAT_FAIL;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (var_tlsmgr_stand_alone && argv[0])
        acl_msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * Initialize. We're select threaded, so we can use static buffers.
     */
    if (request == 0) {
	request = acl_vstring_alloc(10);
	cache_type = acl_vstring_alloc(10);
	cache_id = acl_vstring_alloc(10);
	buffer = acl_vstring_alloc(10);
    }

    /*
     * This routine runs whenever a client connects to the socket dedicated
     * to the tlsmgr service (including wake up events sent by the master).
     * All connection-management stuff is handled by the common code in
     * multi_server.c.
     */
    if (tlsmgr_request_receive(client_stream, request) == 0) {

	/*
	 * Load session from cache.
	 */
	if (STREQ(STR(request), TLS_MGR_REQ_LOOKUP)) {
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_END) == 2) {
		for (ent = cache_table; ent->cache_label; ++ent)
		    if (strcmp(ent->cache_label, STR(cache_type)) == 0)
			break;
		if (ent->cache_label == 0) {
		    acl_msg_warn("%s: bogus cache type \"%s\" in \"%s\" request",
			     myname, STR(cache_type), TLS_MGR_REQ_LOOKUP);
		    ACL_VSTRING_RESET(buffer);
		} else if (ent->cache_info == 0) {

		    /*
		     * Cache type valid, but not enabled
		     */
		    ACL_VSTRING_RESET(buffer);
		} else {
		    status = tls_scache_lookup(ent->cache_info,
					       STR(cache_id), buffer) ?
			TLS_MGR_STAT_OK : TLS_MGR_STAT_ERR;
		}
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, TLS_ATTR_STATUS, status,
		       ATTR_TYPE_DATA, TLS_MGR_ATTR_SESSION,
		       LEN(buffer), STR(buffer),
		       ATTR_TYPE_END);
	}

	/*
	 * Save session to cache.
	 */
	else if (STREQ(STR(request), TLS_MGR_REQ_UPDATE)) {
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SESSION, buffer,
			  ATTR_TYPE_END) == 3) {
		for (ent = cache_table; ent->cache_label; ++ent)
		    if (strcmp(ent->cache_label, STR(cache_type)) == 0)
			break;
		if (ent->cache_label == 0) {
		    acl_msg_warn("%s: bogus cache type \"%s\" in \"%s\" request",
			     myname, STR(cache_type), TLS_MGR_REQ_UPDATE);
		} else if (ent->cache_info != 0) {
		    status = 0;
			tls_scache_update(ent->cache_info, STR(cache_id),
					  STR(buffer), LEN(buffer)) ?
			TLS_MGR_STAT_OK : TLS_MGR_STAT_ERR;
		}
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, TLS_ATTR_STATUS, status,
		       ATTR_TYPE_END);
	}

	/*
	 * Delete session from cache.
	 */
	else if (STREQ(STR(request), TLS_MGR_REQ_DELETE)) {
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_END) == 2) {
		for (ent = cache_table; ent->cache_label; ++ent)
		    if (strcmp(ent->cache_label, STR(cache_type)) == 0)
			break;
		if (ent->cache_label == 0) {
		    acl_msg_warn("%s: bogus cache type \"%s\" in \"%s\" request",
			     myname, STR(cache_type), TLS_MGR_REQ_DELETE);
		} else if (ent->cache_info != 0) {
		    status = tls_scache_delete(ent->cache_info,
					       STR(cache_id)) ?
			TLS_MGR_STAT_OK : TLS_MGR_STAT_ERR;
		}
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, TLS_ATTR_STATUS, status,
		       ATTR_TYPE_END);
	}

	/*
	 * Entropy request.
	 */
	else if (STREQ(STR(request), TLS_MGR_REQ_SEED)) {
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_INT, TLS_MGR_ATTR_SIZE, &len,
			  ATTR_TYPE_END) == 1) {
		ACL_VSTRING_RESET(buffer);
		if (len <= 0 || len > 255) {
		    acl_msg_warn("%s: bogus seed length \"%d\" in \"%s\" request",
			     myname, len, TLS_MGR_REQ_SEED);
		} else {
		    ACL_VSTRING_SPACE(buffer, len);
		    RAND_bytes((unsigned char *) STR(buffer), len);
		    ACL_VSTRING_AT_OFFSET(buffer, len);	/* XXX not part of the
							 * official interface */
		    status = TLS_MGR_STAT_OK;
		}
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, TLS_ATTR_STATUS, status,
		       ATTR_TYPE_DATA, TLS_MGR_ATTR_SEED,
		       LEN(buffer), STR(buffer),
		       ATTR_TYPE_END);
	}

	/*
	 * Caching policy request.
	 */
	else if (STREQ(STR(request), TLS_MGR_REQ_POLICY)) {
	    int     cachable = 0;

	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_END) == 1) {
		for (ent = cache_table; ent->cache_label; ++ent)
		    if (strcmp(ent->cache_label, STR(cache_type)) == 0)
			break;
		if (ent->cache_label == 0) {
		    acl_msg_warn("%s: bogus cache type \"%s\" in \"%s\" request",
			     myname, STR(cache_type), TLS_MGR_REQ_POLICY);
		} else {
		    cachable = (ent->cache_info != 0) ? 1 : 0;
		    status = TLS_MGR_STAT_OK;
		}
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, TLS_ATTR_STATUS, status,
		       ATTR_TYPE_INT, TLS_MGR_ATTR_CACHABLE, cachable,
		       ATTR_TYPE_END);
	}
        
	/*
	 * Master trigger. Normally, these triggers arrive only after some
	 * other process requested the tlsmgr's service. The purpose is to
	 * restart the tlsmgr after it aborted due to a fatal run-time error,
	 * so that it can continue its housekeeping even while nothing is
	 * using TLS.
	 * 
	 * XXX Which begs the question, if TLS isn't used often, do we need a
	 * tlsmgr background process? It could terminate when the session
	 * caches are empty.
	 */
	else if (var_tlsmgr_stand_alone && STREQ(STR(request), wakeup)) {
	    if (acl_msg_verbose)
		acl_msg_info("received master trigger");
#ifdef ACL_UNIX
	    acl_multi_server_disconnect(client_stream);
#endif
	    return;                             /* NOT: acl_vstream_fflush */
	}

	/*
	 * protocol error
	 */
	else {
	    attr_print(client_stream, ATTR_FLAG_NONE,
			ATTR_TYPE_INT, TLS_ATTR_STATUS, TLS_MGR_STAT_FAIL,
			ATTR_TYPE_END);
	}
    }

    /*
     * Protocol error.
     */
    else {
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_INT, TLS_ATTR_STATUS, TLS_MGR_STAT_FAIL,
		   ATTR_TYPE_END);
    }
    acl_vstream_fflush(client_stream);
}

/* tlsmgr_pre_init - pre-jail initialization */

static void tlsmgr_pre_init(char *unused_name acl_unused, char **unused_argv acl_unused)
{
    const char *myname = "tlsmgr_pre_init";
    char   *path;
    struct timeval tv;
    TLSMGR_SCACHE *ent;
    ACL_HTABLE *dup_filter;
    const char *dup_label;

    /*
     * If nothing else works then at least this will get us a few bits of
     * entropy.
     * 
     * XXX This is our first call into the OpenSSL library. We should find out
     * if this can be moved to the post-jail initialization phase, without
     * breaking compatibility with existing installations.
     */
#ifdef WIN32
# define getpid _getpid
#endif
    gettimeofday(&tv, 0);
    tv.tv_sec ^= getpid();
    RAND_seed(&tv, sizeof(struct timeval));

    /*
     * Open the external entropy source. We will not be able to open it again
     * after we are sent to chroot jail, so we keep it open. Errors are not
     * fatal. The exchange file (see below) is the only entropy source that
     * really matters in the long run.
     * 
     * Security note: we open the entropy source while privileged, but we don't
     * access the source until after we release privileges. This way, none of
     * the OpenSSL code gets to execute while we are privileged.
     */
    if (*var_tls_rand_source) {

	/*
	 * Source is a random device.
	 */
	if (!strncmp(var_tls_rand_source, DEV_PREF, DEV_PREF_LEN)) {
	    path = DEV_PATH(var_tls_rand_source);
	    rand_source_dev = tls_prng_dev_open(path, TLS_MGR_TIMEOUT);
	    if (rand_source_dev == 0)
		acl_msg_warn("%s: cannot open entropy device %s: %s",
			myname, path, acl_last_serror());
	}

	/*
	 * Source is an EGD compatible socket.
	 */
	else if (!strncmp(var_tls_rand_source, EGD_PREF, EGD_PREF_LEN)) {
	    path = EGD_PATH(var_tls_rand_source);
	    rand_source_egd = tls_prng_egd_open(path, TLS_MGR_TIMEOUT);
	    if (rand_source_egd == 0)
		acl_msg_warn("%s: cannot connect to EGD server %s: %s",
			myname, path, acl_last_serror());
	}

	/*
	 * Source is regular file. We read this only once.
	 */
	else {
	    rand_source_file =
		tls_prng_file_open(var_tls_rand_source, TLS_MGR_TIMEOUT);
	}
    } else {
	acl_msg_warn("%s: no entropy source specified with parameter %s",
		 myname, VAR_TLS_RAND_SOURCE);
	acl_msg_warn("%s: encryption keys etc. may be predictable", myname);
    }

    /*
     * Open the PRNG exchange file before going to jail, but don't use root
     * privileges. Start the exchange file read/update pseudo thread after
     * dropping privileges.
     */
    if (*var_tls_rand_exch_name) {
	rand_exch = tls_prng_exch_open(var_tls_rand_exch_name);
	if (rand_exch == 0)
	    acl_msg_fatal("cannot open PRNG exchange file %s: %s",
		      var_tls_rand_exch_name, acl_last_serror());
    }

    /*
     * Open the session cache files and discard old information before going
     * to jail, but don't use root privilege. Start the cache maintenance
     * pseudo threads after dropping privileges.
     */

    if (!var_tlsmgr_stand_alone)
	tls_scache_init();

    dup_filter = acl_htable_create(sizeof(cache_table) / sizeof(cache_table[0]), 0);
    for (ent = cache_table; ent->cache_label; ++ent) {
	if (**ent->cache_db) {
	    if ((dup_label = acl_htable_find(dup_filter, *ent->cache_db)) != 0)
		acl_msg_fatal("do not use the same TLS cache file %s for %s and %s",
			  *ent->cache_db, dup_label, ent->cache_label);
	    acl_htable_enter(dup_filter, *ent->cache_db, ent->cache_label);
	    ent->cache_info = tls_scache_open(*ent->cache_db,
					ent->cache_label,
					*ent->cache_loglevel >= 2,
					*ent->cache_timeout);
	}
    }
    acl_htable_free(dup_filter, NULL);
}

/* tlsmgr_post_init - post-jail initialization */

static void tlsmgr_post_init(char *unused_name acl_unused,
	char **unused_argv acl_unused)
{
    const char *myname = "tlsmgr_post_init";
    TLSMGR_SCACHE *ent;

#define NULL_EVENT	(0)
#define NULL_CONTEXT	((char *) 0)

#ifdef ACL_UNIX
    if (var_tlsmgr_stand_alone)
	__eventp = acl_multi_server_event();
    else
#endif
	__eventp = acl_event_new_select(1, 0);

    if (__eventp == NULL)
	acl_msg_fatal("%s: __eventp null", myname);

    if (var_tlsmgr_stand_alone) {
	/*
	 * This routine runs after the skeleton code has entered the chroot jail,
	 * but before any client requests are serviced. Prevent automatic process
	 * suicide after a limited number of client requests or after a limited
	 * amount of idle time.
	 */
#ifdef ACL_UNIX
	acl_var_multi_use_limit = 0;
	acl_var_multi_idle_limit = 0;
#endif
    }

    /*
     * Start the internal PRNG re-seeding pseudo thread first.
     */
    if (*var_tls_rand_source) {
	if (var_tls_reseed_period > INT_MAX / UCHAR_MAX)
	    var_tls_reseed_period = INT_MAX / UCHAR_MAX;
	tlsmgr_reseed_event(NULL_EVENT, NULL, NULL_CONTEXT);
    }

    /*
     * Start the exchange file read/update pseudo thread.
     */
    if (*var_tls_rand_exch_name) {
	if (var_tls_prng_exch_period > INT_MAX / UCHAR_MAX)
	    var_tls_prng_exch_period = INT_MAX / UCHAR_MAX;
	tlsmgr_prng_exch_event(NULL_EVENT, NULL, NULL_CONTEXT);
    }

    /*
     * Start the cache maintenance pseudo threads last. Strictly speaking
     * there is nothing to clean up after we truncate the database to zero
     * length, but early cleanup makes verbose logging more informative (we
     * get positive confirmation that the cleanup threads are running).
     */
    for (ent = cache_table; ent->cache_label; ++ent)
	if (ent->cache_info)
	    tlsmgr_cache_run_event(NULL_EVENT, NULL, (char *) ent);
}

/* tlsmgr_before_exit - save PRNG state before exit */

static void tlsmgr_before_exit(char *unused_service_name acl_unused,
	char **unused_argv acl_unused)
{

    /*
     * Save state before we exit after "postfix reload".
     */
    if (rand_exch)
	tls_prng_exch_update(rand_exch);
}

#ifdef ACL_UNIX

void   tlsmgr_alone_start(int argc, char *argv[])
{
    static ACL_CONFIG_STR_TABLE str_table[] = {
	{ VAR_TLS_RAND_SOURCE, DEF_TLS_RAND_SOURCE, &var_tls_rand_source },
	{ VAR_TLS_RAND_EXCH_NAME, DEF_TLS_RAND_EXCH_NAME, &var_tls_rand_exch_name },
	{ VAR_SERVER_TLS_SCACHE_DB, DEF_SERVER_TLS_SCACHE_DB, &var_server_tls_scache_db },
	{ VAR_CLIENT_TLS_SCACHE_DB, DEF_CLIENT_TLS_SCACHE_DB, &var_client_tls_scache_db },
	{ 0, 0, 0 },
    };
    static ACL_CONFIG_INT_TABLE int_table[] = {
	{ VAR_TLS_RAND_BYTES, DEF_TLS_RAND_BYTES, &var_tls_rand_bytes, 1, 0 },
	{ VAR_SERVER_TLS_LOGLEVEL, DEF_SERVER_TLS_LOGLEVEL, &var_server_tls_loglevel, 0, 0 },
	{ VAR_CLIENT_TLS_LOGLEVEL, DEF_CLIENT_TLS_LOGLEVEL, &var_client_tls_loglevel, 0, 0 },

	{ VAR_TLS_RESEED_PERIOD, DEF_TLS_RESEED_PERIOD, &var_tls_reseed_period, 1, 0 },
	{ VAR_TLS_PRNG_UPD_PERIOD, DEF_TLS_PRNG_UPD_PERIOD, &var_tls_prng_exch_period, 1, 0 },
	{ VAR_SERVER_TLS_SCACHTIME, DEF_SERVER_TLS_SCACHTIME, &var_server_tls_scache_timeout, 0, 0 },
	{ VAR_CLIENT_TLS_SCACHTIME, DEF_CLIENT_TLS_SCACHTIME, &var_client_tls_scache_timeout, 0, 0 },
	{ 0, 0, 0, 0, 0 },
    };

    /*
     * Use the multi service skeleton, and require that no-one else is
     * monitoring our service port while this process runs.
     */
    acl_multi_server_main(argc, argv, tlsmgr_service,
	    ACL_MASTER_SERVER_INT_TABLE, int_table,
	    ACL_MASTER_SERVER_STR_TABLE, str_table,
	    ACL_MASTER_SERVER_PRE_INIT, tlsmgr_pre_init,
	    ACL_MASTER_SERVER_POST_INIT, tlsmgr_post_init,
	    ACL_MASTER_SERVER_EXIT, tlsmgr_before_exit,
	    ACL_MASTER_SERVER_LOOP, tlsmgr_loop,
	    ACL_MASTER_SERVER_SOLITARY,
	    0);
}

#endif  /* ACL_UNIX */

static void client_ready_callback(int event_type acl_unused,
	ACL_EVENT *event, ACL_VSTREAM *stream acl_unused, void *ctx)
{
    ACL_VSTREAM *client_stream = (ACL_VSTREAM*) ctx;

    tlsmgr_service(client_stream, NULL, NULL);
    acl_event_enable_read(event, client_stream,
	0, client_ready_callback, client_stream);
}

static void listen_callback(int event_type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *ctx acl_unused)
{
    const char *myname = "listen_callback";
    ACL_VSTREAM *client_stream;

    client_stream = acl_vstream_accept(stream, NULL, 0);
    if (client_stream == NULL) {
	acl_msg_warn("%s(%d): accept error(%s)",
		myname, __LINE__, acl_last_serror());
	return;
    }
    acl_event_enable_read(event, client_stream,
	0, client_ready_callback, client_stream);
}

static void *tlsmgr_thread_main(void *arg)
{
    ACL_VSTREAM *server_stream = (ACL_VSTREAM*) arg;
    int   delay_sec;

    acl_event_enable_listen(__eventp, server_stream,
	    0, listen_callback, server_stream);
    while (1) {
	delay_sec = tlsmgr_loop(NULL, NULL);
	acl_event_set_delay_sec(__eventp, delay_sec);
	acl_event_loop(__eventp);
    }

    acl_vstream_close(server_stream);
    tlsmgr_before_exit(NULL, NULL);
    return (NULL);
}

void   tlsmgr_local_start(const char *addr)
{
    const char *myname = "tlsmgr_local_start";
    static ACL_CONFIG_STR_TABLE str_table[] = {
	{ VAR_TLS_RAND_SOURCE, DEF_TLS_RAND_SOURCE, &var_tls_rand_source },
	{ VAR_TLS_RAND_EXCH_NAME, DEF_TLS_RAND_EXCH_NAME, &var_tls_rand_exch_name },
	{ VAR_SERVER_TLS_SCACHE_DB, DEF_SERVER_TLS_SCACHE_DB, &var_server_tls_scache_db },
	{ VAR_CLIENT_TLS_SCACHE_DB, DEF_CLIENT_TLS_SCACHE_DB, &var_client_tls_scache_db },
	{ 0, 0, 0 },
    };
    static ACL_CONFIG_INT_TABLE int_table[] = {
	{ VAR_TLS_RAND_BYTES, DEF_TLS_RAND_BYTES, &var_tls_rand_bytes, 1, 0 },
	{ VAR_SERVER_TLS_LOGLEVEL, DEF_SERVER_TLS_LOGLEVEL, &var_server_tls_loglevel, 0, 0 },
	{ VAR_CLIENT_TLS_LOGLEVEL, DEF_CLIENT_TLS_LOGLEVEL, &var_client_tls_loglevel, 0, 0 },

	{ VAR_TLS_RESEED_PERIOD, DEF_TLS_RESEED_PERIOD, &var_tls_reseed_period, 1, 0 },
	{ VAR_TLS_PRNG_UPD_PERIOD, DEF_TLS_PRNG_UPD_PERIOD, &var_tls_prng_exch_period, 1, 0 },
	{ VAR_SERVER_TLS_SCACHTIME, DEF_SERVER_TLS_SCACHTIME, &var_server_tls_scache_timeout, 0, 0 },
	{ VAR_CLIENT_TLS_SCACHTIME, DEF_CLIENT_TLS_SCACHTIME, &var_client_tls_scache_timeout, 0, 0 },
	{ 0, 0, 0, 0, 0 },
    };
    acl_pthread_t tid;
    acl_pthread_attr_t attr;
    char  def_addr[] = "127.0.0.1:0";
    ACL_VSTREAM *server_stream;

    acl_get_app_conf_str_table(str_table);
    acl_get_app_conf_int_table(int_table);

    server_stream = acl_vstream_listen((addr == NULL || *addr == 0) ? def_addr : addr, 128);
    if (server_stream == NULL) {
    	tlsmgr_before_exit(NULL, NULL);
	acl_msg_error("%s(%d): listen addr(%s) error(%s)",
		myname, __LINE__, addr, acl_last_serror());
	return;
    }
    snprintf(var_tlsmgr_service, sizeof(var_tlsmgr_service),
	"inet:%s", ACL_VSTREAM_LOCAL(server_stream));

    tlsmgr_pre_init(NULL, NULL);
    tlsmgr_post_init(NULL, NULL);

    acl_pthread_attr_init(&attr);
#ifdef ACL_UNIX
    acl_pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#else
    acl_pthread_attr_setdetachstate(&attr, ACL_PTHREAD_CREATE_DETACHED);
#endif

    acl_pthread_create(&tid, &attr, tlsmgr_thread_main, server_stream);
}

#endif  /* USE_TLS */

