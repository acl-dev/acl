/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_split_at.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_stringops.h"

#include "net/acl_valid_hostname.h"

/* Global library. */

#include "net/acl_host_port.h"

#endif

/* host_port - parse string into host and port, destroy string */

const char *acl_host_port(char *buf, char **host, char *def_host,
	char **port, char *def_service)
{
	char *cp = buf;

	/* port */
	if (acl_alldig(buf)) {
		*port = buf;
		*host = def_host;
	}
	/* [host]:port, [host]:, [host] */
	else if (*cp == '[') {
		*host = ++cp;
		if ((cp = acl_split_at(cp, ']')) == 0)
			return "missing \"]\"";
		if (*cp && *cp++ != ':')
			return "garbage after \"]\"";
		*port = *cp ? cp : def_service;
	}
	/* host#port, host#, host, #port */
	else if ((cp = acl_split_at_right(buf, ACL_ADDR_SEP)) != 0) {
		*host = *buf ? buf : def_host;
		*port = *cp ? cp : def_service;
	}
	/* host:port, host:, host, :port */
	else if ((cp = acl_split_at_right(buf, ':')) != 0) {
		*host = *buf ? buf : def_host;
		*port = *cp ? cp : def_service;
	} else {
		*host = *buf ? buf : (def_host ? def_host : NULL);
		*port = def_service ? def_service : NULL;
	}

	if (*host == 0)
		return "missing host information";

	/*
	 * if (*port == 0)
	 *	return "missing service information";
	 */

	/*
	 * Final sanity checks. We're still sloppy, allowing bare numerical
	 * network addresses instead of requiring proper [ipaddress] forms.
	 */
	if (*host != def_host
	    && !acl_valid_hostname(*host, ACL_DONT_GRIPE)
	    && !acl_valid_hostaddr(*host, ACL_DONT_GRIPE)) {

		return "valid hostname or network address required";
	}

	if (*port == 0)
		return NULL;

	if (*port != def_service && ACL_ISDIGIT(**port) && !acl_alldig(*port))
		return "garbage after numerical service";

	return NULL;
}

static int host_port(char *buf, char **host, char **port)
{
	char *def_host = "";
	const char *ptr = acl_host_port(buf, host, def_host, port, NULL);

	if (ptr != NULL) {
		acl_msg_error("%s(%d), %s: invalid addr %s, %s",
			__FILE__, __LINE__, __FUNCTION__, buf, ptr);
		return -1;
	}

	if (*port != NULL && atoi(*port) < 0) {
		acl_msg_error("%s(%d), %s: invalid port: %s, addr: %s",
			__FILE__, __LINE__, __FUNCTION__,
			*port ? *port : "null", buf);
		return -1;
	}

	if (*host && **host == 0)
		*host = 0;
	if (*host == NULL)
#if defined(AF_INET6) && !defined(ACL_WINDOWS)
		*host = "0";
#else
		*host = "0.0.0.0";
#endif

	return 0;
}

struct addrinfo *acl_host_addrinfo(const char *addr, int type)
{
	int    err;
	struct addrinfo hints, *res0;
	char  *buf = acl_mystrdup(addr);
	char *host = NULL, *port = NULL;

	if (host_port(buf, &host, &port) < 0) {
		acl_myfree(buf);
		return NULL;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = type;
#ifdef	ACL_MACOSX
	hints.ai_flags    = AI_DEFAULT;
#elif	defined(ACL_ANDROID)
	hints.ai_flags    = AI_ADDRCONFIG;
#elif	defined(ACL_WINDOWS)
	hints.ai_protocol = type == SOCK_DGRAM ? IPPROTO_UDP : IPPROTO_TCP;
# if _MSC_VER >= 1500
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
# endif
#elif	!defined(ACL_FREEBSD)
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
#endif
	if ((err = getaddrinfo(host, port, &hints, &res0))) {
		acl_msg_error("%s(%d): getaddrinfo error %s, host=%s, addr=%s",
			__FILE__, __LINE__, gai_strerror(err), host, addr);
		acl_myfree(buf);
		return NULL;
	}

	acl_myfree(buf);
	return res0;
}
