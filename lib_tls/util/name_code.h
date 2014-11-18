#ifndef _NAME_CODE_H_INCLUDED_
#define _NAME_CODE_H_INCLUDED_

/*++
 * NAME
 *	name_mask 3h
 * SUMMARY
 *	name to number table mapping
 * SYNOPSIS
 *	#include <name_code.h>
 * DESCRIPTION
 * .nf

  *
  * External interface.
  */
typedef struct {
    const char *name;
    int     code;
} NAME_CODE;

#define NAME_CODE_FLAG_NONE		0
#define NAME_CODE_FLAG_STRICT_CASE	(1<<0)

extern int name_code(const NAME_CODE *, int, const char *);
extern const char *str_name_code(const NAME_CODE *, int);

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

