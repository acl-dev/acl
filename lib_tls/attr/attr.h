#ifndef _ATTR_H_INCLUDED_
#define _ATTR_H_INCLUDED_

/*++
 * NAME
 *	attr 3h
 * SUMMARY
 *	attribute list manipulations
 * SYNOPSIS
 *	#include "attr.h"
 DESCRIPTION
 .nf
 */

/*
 *
 * System library.
 */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include "lib_acl.h"

 /*
  * Attribute types. See attr_scan(3) for documentation.
  */
#define ATTR_TYPE_END		0	/* end of data */
#define ATTR_TYPE_INT		1	/* Unsigned integer */
#define ATTR_TYPE_NUM		ATTR_TYPE_INT
#define ATTR_TYPE_STR		2	/* Character string */
#define ATTR_TYPE_HASH		3	/* Hash table */
#define ATTR_TYPE_NV		3	/* Name-value table */
#define ATTR_TYPE_LONG		4	/* Unsigned long */
#define ATTR_TYPE_DATA		5	/* Binary data */
#define ATTR_TYPE_FUNC		6	/* Function pointer */

#define ATTR_HASH_LIMIT		1024	/* Size of hash table */

 /*
  * Flags that control processing. See attr_scan(3) for documentation.
  */
#define ATTR_FLAG_NONE		0
#define ATTR_FLAG_MISSING	(1<<0)	/* Flag missing attribute */
#define ATTR_FLAG_EXTRA		(1<<1)	/* Flag spurious attribute */
#define ATTR_FLAG_MORE		(1<<2)	/* Don't skip or terminate */

#define ATTR_FLAG_STRICT	(ATTR_FLAG_MISSING | ATTR_FLAG_EXTRA)
#define ATTR_FLAG_ALL		(07)

 /*
  * Delegation for better data abstraction.
  */
typedef int (*ATTR_SCAN_MASTER_FN) (ACL_VSTREAM *, int, ...);
typedef int (*ATTR_SCAN_SLAVE_FN) (ATTR_SCAN_MASTER_FN, ACL_VSTREAM *, int, void *);
typedef int (*ATTR_PRINT_MASTER_FN) (ACL_VSTREAM *, int,...);
typedef int (*ATTR_PRINT_SLAVE_FN) (ATTR_PRINT_MASTER_FN, ACL_VSTREAM *, int, void *);

 /*
  * Default to null-terminated, as opposed to base64-encoded.
  */
#define attr_print	attr_print0
#define attr_vprint	attr_vprint0
#define attr_scan	attr_scan0
#define attr_vscan	attr_vscan0

 /*
  * attr_print64.c.
  */
extern int attr_print64(ACL_VSTREAM *, int,...);
extern int attr_vprint64(ACL_VSTREAM *, int, va_list);

 /*
  * attr_scan64.c.
  */
extern int attr_scan64(ACL_VSTREAM *, int,...);
extern int attr_vscan64(ACL_VSTREAM *, int, va_list);

 /*
  * attr_print0.c.
  */
extern int attr_print0(ACL_VSTREAM *, int,...);
extern int attr_vprint0(ACL_VSTREAM *, int, va_list);

 /*
  * attr_scan0.c.
  */
extern int attr_scan0(ACL_VSTREAM *, int,...);
extern int attr_vscan0(ACL_VSTREAM *, int, va_list);

 /*
  * attr_scan_plain.c.
  */
extern int attr_print_plain(ACL_VSTREAM *, int,...);
extern int attr_vprint_plain(ACL_VSTREAM *, int, va_list);

 /*
  * attr_print_plain.c.
  */
extern int attr_scan_plain(ACL_VSTREAM *, int,...);
extern int attr_vscan_plain(ACL_VSTREAM *, int, va_list);


 /*
  * Attribute names for testing the compatibility of the read and write
  * routines.
  */
#ifdef TEST
#define ATTR_NAME_INT		"number"
#define ATTR_NAME_STR		"string"
#define ATTR_NAME_LONG		"long_number"
#define ATTR_NAME_DATA		"data"
#endif

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

#endif
