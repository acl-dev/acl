/*++
 * NAME
 *	tok822_node 3
 * SUMMARY
 *	token memory management
 * SYNOPSIS
 *	#include <tok822.h>
 *
 *	TOK822	*tok822_alloc(type, strval)
 *	int	type;
 *	const char *strval;
 *
 *	TOK822	*tok822_free(tp)
 *	TOK822	*tp;
 * DESCRIPTION
 *	This module implements memory management for token
 *	structures. A distinction is made between single-character
 *	tokens that have no string value, and string-valued tokens.
 *
 *	tok822_alloc() allocates memory for a token structure of
 *	the named type, and initializes it properly. In case of
 *	a single-character token, no string memory is allocated.
 *	Otherwise, \fIstrval\fR is a null pointer or provides
 *	string data to initialize the token with.
 *
 *	tok822_free() releases the memory used for the specified token
 *	and conveniently returns a null pointer value.
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

#include <string.h>

/* Utility library. */

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_vstring.h"

/* Global library. */

#include "tok822.hpp"

/* tok822_alloc - allocate and initialize token */

TOK822 *tok822_alloc(int type, const char *strval)
{
	TOK822 *tp;

#define CONTAINER_TOKEN(x) \
	((x) == TOK822_ADDR || (x) == TOK822_STARTGRP)

	tp = (TOK822 *) acl_mymalloc(sizeof(*tp));
	tp->type = type;
	tp->next = tp->prev = tp->head = tp->tail = tp->owner = 0;
	tp->vstr = (type < TOK822_MINTOK || CONTAINER_TOKEN(type) ? 0 :
		strval == 0 ? acl_vstring_alloc(10) :
		acl_vstring_strcpy(acl_vstring_alloc(strlen(strval) + 1), strval));
	return (tp);
}

/* tok822_free - destroy token */

TOK822 *tok822_free(TOK822 *tp)
{
	if (tp->vstr)
		acl_vstring_free(tp->vstr);
	acl_myfree(tp);
	return (0);
}

#endif // !defined(ACL_MIME_DISABLE)
