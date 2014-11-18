/*++
 * NAME
 *	tok822_rewrite 3
 * SUMMARY
 *	address rewriting, client interface
 * SYNOPSIS
 *	#include <tok822.h>
 *
 *	TOK822	*tok822_rewrite(addr, how)
 *	TOK822	*addr;
 *	char	*how;
 * DESCRIPTION
 *	tok822_rewrite() takes an address token tree and transforms
 *	it according to the rule set specified via \fIhow\fR. The
 *	result is the \fIaddr\fR argument.
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

/* Utility library. */
#include "acl_stdafx.hpp"
#include "acl/stdlib/acl_vstring.h"
#include "acl/stdlib/acl_msg.h"

/* Global library. */

#include "rewrite_clnt.h"
#include "tok822.h"

/* tok822_rewrite - address rewriting interface */

#define STR	acl_vstring_str

TOK822 *tok822_rewrite(TOK822 *addr, const char *how)
{
	const char *myname = "tok822_rewrite";
	ACL_VSTRING *input_ext_form = acl_vstring_alloc(100);
	ACL_VSTRING *canon_ext_form = acl_vstring_alloc(100);

	if (addr->type != TOK822_ADDR)
		acl_msg_panic("%s: non-address token type: %d", myname, addr->type);

	/*
	 * Externalize the token tree, ship it to the rewrite service, and parse
	 * the result. Shipping external form is much simpler than shipping parse
	 * trees.
	 */
	tok822_externalize(input_ext_form, addr->head, TOK822_STR_DEFL);
	if (acl_msg_verbose)
		acl_msg_info("%s: input: %s", myname, STR(input_ext_form));
	rewrite_clnt(how, STR(input_ext_form), canon_ext_form);
	if (msg_verbose)
		acl_msg_info("tok822_rewrite: result: %s", STR(canon_ext_form));
	tok822_free_tree(addr->head);
	addr->head = tok822_scan(STR(canon_ext_form), &addr->tail);

	acl_vstring_free(input_ext_form);
	acl_vstring_free(canon_ext_form);
	return (addr);
}
