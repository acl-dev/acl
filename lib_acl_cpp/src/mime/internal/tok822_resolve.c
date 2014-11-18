/*++
 * NAME
 *	tok822_resolve 3
 * SUMMARY
 *	address resolving, client interface
 * SYNOPSIS
 *	#include <tok822.h>
 *
 *	void	tok822_resolve(addr, reply)
 *	TOK822	*addr;
 *	RESOLVE_REPLY *reply;
 *
 *	void	tok822_resolve_from(sender, addr, reply)
 *	const char *sender;
 *	TOK822	*addr;
 *	RESOLVE_REPLY *reply;
 * DESCRIPTION
 *	tok822_resolve() takes an address token tree and finds out the
 *	transport to deliver via, the next-hop host on that transport,
 *	and the recipient relative to that host.
 *
 *	tok822_resolve_from() allows the caller to specify sender context
 *	that will be used to look up sender-dependent relayhost information.
 * SEE ALSO
 *	resolve_clnt(3) basic resolver client interface
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

/* System library. */
#include "acl_stdafx.hpp"

/* Utility library. */

#include "acl/stdlib/acl_vstring.h"
#include "acl/stdlib/acl_msg.h"

/* Global library. */

#include "resolve_clnt.h"
#include "tok822.h"

/* tok822_resolve - address rewriting interface */

void    tok822_resolve_from(const char *sender, TOK822 *addr,
		RESOLVE_REPLY *reply)
{
	ACL_VSTRING *intern_form = acl_vstring_alloc(100);

	if (addr->type != TOK822_ADDR)
		acl_msg_panic("tok822_resolve: non-address token type: %d", addr->type);

	/*
	 * Internalize the token tree and ship it to the resolve service.
	 * Shipping string forms is much simpler than shipping parse trees.
	 */
	tok822_internalize(intern_form, addr->head, TOK822_STR_DEFL);
	resolve_clnt_query_from(sender, acl_vstring_str(intern_form), reply);
	if (acl_msg_verbose)
		acl_msg_info("tok822_resolve: from=%s addr=%s -> chan=%s, host=%s, rcpt=%s",
			sender,
			acl_vstring_str(intern_form), acl_vstring_str(reply->transport),
			acl_vstring_str(reply->nexthop), acl_vstring_str(reply->recipient));

	acl_vstring_free(intern_form);
}
