/*++
 * NAME
 *	tok822_find 3
 * SUMMARY
 *	token list search operators
 * SYNOPSIS
 *	#include <tok822.h>
 *
 *	TOK822	*tok822_find_type(head, type)
 *	TOK822	*head;
 *	int	type;
 *
 *	TOK822	*tok822_rfind_type(tail, type)
 *	TOK822	*tail;
 *	int	type;
 * DESCRIPTION
 *	This module implements token list search operations.
 *
 *	tok822_find_type() searches a list of tokens for the first
 *	instance of the specified token type. The result is the
 *	found token or a null pointer when the search failed.
 *
 *	tok822_rfind_type() searches a list of tokens in reverse direction
 *	for the first instance of the specified token type. The result
 *	is the found token or a null pointer when the search failed.
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

#if !defined(ACL_MIME_DISABLE)

/* Utility library. */

/* Global library. */

#include "tok822.hpp"

/* tok822_find_type - find specific token type, forward search */

TOK822 *tok822_find_type(TOK822 *head, int op)
{
	TOK822 *tp;

	for (tp = head; tp != 0 && tp->type != op; tp = tp->next)
		/* void */ ;
	return (tp);
}

/* tok822_rfind_type - find specific token type, backward search */

TOK822 *tok822_rfind_type(TOK822 *tail, int op)
{
	TOK822 *tp;

	for (tp = tail; tp != 0 && tp->type != op; tp = tp->prev)
		/* void */ ;
	return (tp);
}

#endif // !defined(ACL_MIME_DISABLE)
