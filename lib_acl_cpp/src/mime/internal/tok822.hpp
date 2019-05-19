#ifndef _TOK822_H_INCLUDED_
#define _TOK822_H_INCLUDED_

#if !defined(ACL_MIME_DISABLE)

/*++
 * NAME
 *	tok822 3h
 * SUMMARY
 *	RFC822 token structures
 * SYNOPSIS
 *	#include <tok822.h>
 * DESCRIPTION
 * .nf
 */
/*
 *
 * Utility library.
 */
#include "stdlib/acl_vstring.h"

/*
 * Global library.
 */
//#include <resolve_clnt.h>

/*
 * Internal address representation: a token tree.
 */
typedef struct TOK822 {
	int     type;               /* token value, see below */
	ACL_VSTRING *vstr;          /* token contents */
	struct TOK822 *prev;        /* peer */
	struct TOK822 *next;        /* peer */
	struct TOK822 *head;        /* group members */
	struct TOK822 *tail;        /* group members */
	struct TOK822 *owner;       /* group owner */
} TOK822;

/*
 * Token values for multi-character objects. Single-character operators are
 * represented by their own character value.
 */
#define TOK822_MINTOK   256
#define	TOK822_ATOM     256		/* non-special character sequence */
#define	TOK822_QSTRING  257		/* stuff between "", not nesting */
#define	TOK822_COMMENT  258		/* comment including (), may nest */
#define	TOK822_DOMLIT   259		/* stuff between [] not nesting */
#define	TOK822_ADDR     260		/* actually a token group */
#define TOK822_STARTGRP 261		/* start of named group */
#define TOK822_MAXTOK   261

/*
 * tok822_node.c
 */
extern TOK822 *tok822_alloc(int, const char *);
extern TOK822 *tok822_free(TOK822 *);

/*
 * tok822_tree.c
 */
extern TOK822 *tok822_append(TOK822 *, TOK822 *);
extern TOK822 *tok822_prepend(TOK822 *, TOK822 *);
extern TOK822 *tok822_cut_before(TOK822 *);
extern TOK822 *tok822_cut_after(TOK822 *);
extern TOK822 *tok822_unlink(TOK822 *);
extern TOK822 *tok822_sub_append(TOK822 *, TOK822 *);
extern TOK822 *tok822_sub_prepend(TOK822 *, TOK822 *);
extern TOK822 *tok822_sub_keep_before(TOK822 *, TOK822 *);
extern TOK822 *tok822_sub_keep_after(TOK822 *, TOK822 *);
extern TOK822 *tok822_free_tree(TOK822 *);

typedef int (*TOK822_ACTION) (TOK822 *);
extern int tok822_apply(TOK822 *, int, TOK822_ACTION);
extern TOK822 **tok822_grep(TOK822 *, int);

/*
 * tok822_parse.c
 */
extern TOK822 *tok822_scan_limit(const char *, TOK822 **, int);
extern TOK822 *tok822_scan_addr(const char *);
extern TOK822 *tok822_parse_limit(const char *, int);
extern ACL_VSTRING *tok822_externalize(ACL_VSTRING *, TOK822 *, int);
extern ACL_VSTRING *tok822_internalize(ACL_VSTRING *, TOK822 *, int);

#define tok822_scan(cp, ptr)	tok822_scan_limit((cp), (ptr), 0)
#define tok822_parse(cp)	tok822_parse_limit((cp), 0)

#define TOK822_STR_NONE	(0)
#define TOK822_STR_WIPE	(1<<0)
#define TOK822_STR_TERM	(1<<1)
#define TOK822_STR_LINE	(1<<2)
#define TOK822_STR_TRNC	(1<<3)
#define TOK822_STR_DEFL	(TOK822_STR_WIPE | TOK822_STR_TERM)
#define TOK822_STR_HEAD	(TOK822_STR_TERM | TOK822_STR_LINE | TOK822_STR_TRNC)

/*
 * tok822_find.c
 */
extern TOK822 *tok822_find_type(TOK822 *, int);
extern TOK822 *tok822_rfind_type(TOK822 *, int);

/*
 * tok822_rewrite.c
 */
extern TOK822 *tok822_rewrite(TOK822 *, const char *);

/*
 * tok822_resolve.c
 */
//#define tok822_resolve(t, r) tok822_resolve_from(RESOLVE_NULL_FROM, (t), (r))
//
//extern void tok822_resolve_from(const char *, TOK822 *, RESOLVE_REPLY *);

/* LICENSE
 * .ad
 * .fi
 *	The Secure Mailer license must be distributed with this software.
 * AUTHOR(S)
 *	Wietse Venema
 *	IBM T.J. Watson Research
 *	P.O. Box 704
 *	Yorktown Heights, NY 10598, USA
 *--*/

#endif // !defined(ACL_MIME_DISABLE)
#endif
