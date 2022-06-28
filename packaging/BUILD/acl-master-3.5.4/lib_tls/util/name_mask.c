/*++
 * NAME
 *	name_mask 3
 * SUMMARY
 *	map names to bit mask
 * SYNOPSIS
 *	#include <name_mask.h>
 *
 *	int	name_mask(context, table, names)
 *	const char *context;
 *	const NAME_MASK *table;
 *	const char *names;
 *
 *	const char *str_name_mask(context, table, mask)
 *	const char *context;
 *	const NAME_MASK *table;
 *	int	mask;
 *
 *	int	name_mask_opt(context, table, names, flags)
 *	const char *context;
 *	const NAME_MASK *table;
 *	const char *names;
 *	int	flags;
 *
 *	int	name_mask_delim_opt(context, table, names, delim, flags)
 *	const char *context;
 *	const NAME_MASK *table;
 *	const char *names;
 *	const char *delim;
 *	int	flags;
 *
 *	const char *str_name_mask_opt(buf, context, table, mask, flags)
 *	VSTRING	*buf;
 *	const char *context;
 *	const NAME_MASK *table;
 *	int	mask;
 *	int	flags;
 * DESCRIPTION
 *	name_mask() takes a null-terminated \fItable\fR with (name, mask)
 *	values and computes the bit-wise OR of the masks that correspond
 *	to the names listed in the \fInames\fR argument, separated by
 *	comma and/or whitespace characters.
 *
 *	str_name_mask() translates a mask into its equivalent names.
 *	The result is written to a static buffer that is overwritten
 *	upon each call.
 *
 *	name_mask_opt() and str_name_mask_opt() are extended versions
 *	with additional fine control. name_mask_delim_opt() supports
 *	non-default delimiter characters.
 *
 *	Arguments:
 * .IP buf
 *	Null pointer or pointer to buffer storage.
 * .IP context
 *	What kind of names and
 *	masks are being manipulated, in order to make error messages
 *	more understandable. Typically, this would be the name of a
 *	user-configurable parameter.
 * .IP table
 *	Table with (name, bit mask) pairs.
 * .IP names
 *	A list of names that is to be converted into a bit mask.
 * .IP mask
 *	A bit mask.
 * .IP flags
 *	Bit-wise OR of zero or more of the following:
 * .IP delim
 *	Delimiter characters to use instead of whitespace and commas.
 * .RS
 * .IP NAME_MASK_FATAL
 *	Require that all names listed in \fIname\fR exist in \fItable\fR,
 *	and that all bits listed in \fImask\fR exist in \fItable\fR.
 *	Terminate with a fatal run-time error if this condition is not met.
 *	This feature is enabled by default when calling name_mask()
 *	or str_name_mask().
 * .IP NAME_MASK_RETURN
 *	Require that all names listed in \fIname\fR exist in \fItable\fR,
 *	and that all bits listed in \fImask\fR exist in \fItable\fR.
 *	Log a warning, and return 0 (name_mask()) or a null pointer
 *	(str_name_mask()) if this condition is not met.
 * .IP NAME_MASK_NUMBER
 *	Require that all bits listed in \fImask\fR exist in \fItable\fR.
 *	For unrecognized bits, print the numerical hexadecimal form.
 * .IP NAME_MASK_ANY_CASE
 *	Enable case-insensitive matching.
 *	This feature is not enabled by default when calling name_mask();
 *	it has no effect with str_name_mask().
 * .IP NAME_MASK_COMMA
 *	Use comma instead of space when converting a mask to string.
 * .IP NAME_MASK_PIPE
 *	Use "|" instead of space when converting a mask to string.
 * .RE
 *	The value NAME_MASK_NONE explicitly requests no features,
 *	and NAME_MASK_DEFAULT enables the default options.
 * DIAGNOSTICS
 *	Fatal: the \fInames\fR argument specifies a name not found in
 *	\fItable\fR, or the \fImask\fR specifies a bit not found in
 *	\fItable\fR.
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

#include "StdAfx.h"
#include <string.h>

/* Utility library. */

#include "name_mask.h"

#define STR(x) acl_vstring_str(x)

/* name_mask_delim_opt - compute mask corresponding to list of names */

int     name_mask_delim_opt(const char *context, const NAME_MASK *table,
		const char *names, const char *delim, int flags)
{
	const char *myname = "name_mask";
	char   *saved_names = acl_mystrdup(names);
	char   *bp = saved_names;
	int     result = 0;
	const NAME_MASK *np;
	char   *name;
	int     (*lookup) (const char *, const char *);

	if (flags & NAME_MASK_ANY_CASE)
		lookup = strcasecmp;
	else
		lookup = strcmp;

	/*
	 * Break up the names string, and look up each component in the table. If
	 * the name is found, merge its mask with the result.
	 */
	while ((name = acl_mystrtok(&bp, delim)) != 0) {
		for (np = table; /* void */ ; np++) {
			if (np->name == 0) {
				if (flags & NAME_MASK_FATAL)
					acl_msg_fatal("unknown %s value \"%s\" in \"%s\"",
						context, name, names);
				if (flags & NAME_MASK_RETURN) {
					acl_msg_warn("unknown %s value \"%s\" in \"%s\"",
						context, name, names);
					return (0);
				}
				break;
			}
			if (lookup(name, np->name) == 0) {
				if (acl_msg_verbose)
					acl_msg_info("%s: %s", myname, name);
				result |= np->mask;
				break;
			}
		}
	}
	acl_myfree(saved_names);
	return (result);
}

static void free_vstring(void *arg)
{
	ACL_VSTRING *s = (ACL_VSTRING*) arg;

	acl_vstring_free(s);
}

/* str_name_mask_opt - mask to string */

const char *str_name_mask_opt(ACL_VSTRING *buf, const char *context,
		const NAME_MASK *table,
		int mask, int flags)
{
	const char *myname = "name_mask";
	const NAME_MASK *np;
	int     len;
	static __thread ACL_VSTRING *my_buf = 0;
	int     delim = (flags & NAME_MASK_COMMA ? ',' :
			(flags & NAME_MASK_PIPE ? '|' : ' '));

	if (buf == 0) {
		if (my_buf == 0) {
			my_buf = acl_vstring_alloc(1);
			acl_pthread_atexit_add(my_buf, free_vstring);
		}
		buf = my_buf;
	}
	ACL_VSTRING_RESET(buf);

	for (np = table; mask != 0; np++) {
		if (np->name == 0) {
			if (flags & NAME_MASK_FATAL) {
				acl_msg_fatal("%s: unknown %s bit in mask: 0x%x",
					myname, context, mask);
			} else if (flags & NAME_MASK_RETURN) {
				acl_msg_warn("%s: unknown %s bit in mask: 0x%x",
					myname, context, mask);
				return (0);
			} else if (flags & NAME_MASK_NUMBER) {
				acl_vstring_sprintf_append(buf, "0x%x%c", mask, delim);
			}
			break;
		}
		if (mask & np->mask) {
			mask &= ~np->mask;
			acl_vstring_sprintf_append(buf, "%s%c", np->name, delim);
		}
	}
	if ((len = (int) ACL_VSTRING_LEN(buf)) > 0)
		acl_vstring_truncate(buf, len - 1);
	ACL_VSTRING_TERMINATE(buf);

	return (STR(buf));
}

#ifdef TEST

 /*
  * Stand-alone test program.
  */
#include <stdlib.h>
#include "stdlib/acl_vstream.h"

int     main(int argc, char **argv)
{
	static const NAME_MASK table[] = {
		"zero", 1 << 0,
		"one", 1 << 1,
		"two", 1 << 2,
		"three", 1 << 3,
		0, 0,
	};
	int     mask;
	ACL_VSTRING *buf = acl_vstring_alloc(1);

	while (--argc && *++argv) {
		mask = name_mask("test", table, *argv);
		acl_vstream_printf("%s -> 0x%x -> %s\n",
			*argv, mask, str_name_mask("mask_test", table, mask));
		acl_vstream_fflush(VSTREAM_OUT);
	}
	acl_vstring_free(buf);
	exit(0);
}

#endif

