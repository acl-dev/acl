#include "stdafx.h"
#include "dns/dns.h"
#include "common/pthread_patch.h"
#include "hook.h"

#ifdef SYS_UNIX

struct dns_resolv_conf *var_dns_conf = NULL;
struct dns_hosts *var_dns_hosts      = NULL;
struct dns_hints *var_dns_hints      = NULL;

void fiber_dns_set_read_wait(int timeout)
{
	set_read_timeout(timeout);
}

static void dns_on_exit(void)
{
	if (var_dns_conf) {
		dns_resconf_close(var_dns_conf);
		var_dns_conf = NULL;
	}

	if (var_dns_hosts) {
		dns_hosts_close(var_dns_hosts);
		var_dns_hosts = NULL;
	}

	if (var_dns_hints) {
		dns_hints_close(var_dns_hints);
		var_dns_hints = NULL;
	}
}

void fiber_dns_init(void)
{
#ifdef SYS_WIN
	static pthread_mutex_t __lock;
#elif defined(SYS_UNIX)
	static pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
#endif
	static int __called = 0;
	int err;

	(void) pthread_mutex_lock(&__lock);

	if (__called) {
		(void) pthread_mutex_unlock(&__lock);
		return;
	}

	__called++;

	err = 0;
	var_dns_conf  = dns_resconf_local(&err);
	assert(var_dns_conf && err == 0);
	var_dns_conf->options.timeout = 1000;

	var_dns_hosts = dns_hosts_local(&err);
	assert(var_dns_hosts && err == 0);

	var_dns_hints = dns_hints_local(var_dns_conf, &err);
	assert(var_dns_hints && err == 0);

	atexit(dns_on_exit);

	(void) pthread_mutex_unlock(&__lock);
}

#endif
