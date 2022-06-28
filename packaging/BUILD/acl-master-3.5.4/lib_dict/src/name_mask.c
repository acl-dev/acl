#include "StdAfx.h"
#include <string.h>
#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

#include "debug_var.h"
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
				acl_debug(DEBUG_NAME_MASK, 1) ("%s: %s", myname, name);
				result |= np->mask;
				break;
			}
		}
	}
	acl_myfree(saved_names);
	return (result);
}

/* str_name_mask_opt - mask to string */

const char *str_name_mask_opt(ACL_VSTRING *buf, const char *context,
		const NAME_MASK *table,
		int mask, int flags)
{
	const char *myname = "name_mask";
	const NAME_MASK *np;
	int     len;
	static ACL_VSTRING *my_buf = 0;
	int     delim = (flags & NAME_MASK_COMMA ? ',' :
			(flags & NAME_MASK_PIPE ? '|' : ' '));

	if (buf == 0) {
		if (my_buf == 0)
			my_buf = acl_vstring_alloc(1);
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
