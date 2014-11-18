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
#include "stdlib/acl_stringops.h"

#include "net/acl_valid_hostname.h"

/* Global library. */

#include "net/acl_host_port.h"

#endif

/* host_port - parse string into host and port, destroy string */

const char *acl_host_port(char *buf, char **host, char *def_host,
	char **port, char *def_service)
{
	char   *cp = buf;

	/*
	 * [host]:port, [host]:, [host].
	 */
	if (*cp == '[') {
		*host = ++cp;
		if ((cp = acl_split_at(cp, ']')) == 0)
			return "missing \"]\"";
		if (*cp && *cp++ != ':')
			return "garbage after \"]\"";
		*port = *cp ? cp : def_service;
	}

	/*
	 * host:port, host:, host, :port, port.
	 */
	else {
		if ((cp = acl_split_at_right(buf, ':')) != 0) {
			*host = *buf ? buf : def_host;
			*port = *cp ? cp : def_service;
		} else {
			*host = def_host ? def_host : (*buf ? buf : 0);
			*port = def_service ? def_service : (*buf ? buf : 0);
		}
	}
	if (*host == 0)
		return "missing host information";
	if (*port == 0)
		return "missing service information";

	/*
	 * Final sanity checks. We're still sloppy, allowing bare numerical
	 * network addresses instead of requiring proper [ipaddress] forms.
	 */
	if (*host != def_host
	    && !acl_valid_hostname(*host, ACL_DONT_GRIPE)
	    && !acl_valid_hostaddr(*host, ACL_DONT_GRIPE))
		return "valid hostname or network address required";
	if (*port != def_service && ACL_ISDIGIT(**port) && !acl_alldig(*port))
		return "garbage after numerical service";
	return NULL;
}
