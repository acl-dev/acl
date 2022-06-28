#include "StdAfx.h"
#include <ctype.h>
#include <string.h>
#include "mac_parse.h"
#include "mac_expand.h"

 /*
  * Little helper structure.
  */
typedef struct {
    ACL_VSTRING *result;		/* result buffer */
    int     flags;			/* features */
    const char *filter;			/* character filter */
    MAC_EXP_LOOKUP_FN lookup;		/* lookup routine */
    char   *context;			/* caller context */
    int     status;			/* findings */
    int     level;			/* nesting level */
} MAC_EXP;

/* mac_expand_callback - callback for mac_parse */

static int mac_expand_callback(int type, ACL_VSTRING *buf, void *ptr)
{
    MAC_EXP *mc = (MAC_EXP *) ptr;
    int     lookup_mode;
    const char *text;
    char   *cp, *pp = NULL;
    int     ch;
    ssize_t len;
    size_t  size;

    /*
     * Sanity check.
     */
    if (mc->level++ > 100) {
	acl_msg_warn("unreasonable macro call nesting: \"%s\"", acl_vstring_str(buf));
	mc->status |= MAC_PARSE_ERROR;
    }
    if (mc->status & MAC_PARSE_ERROR)
	return (mc->status);

    /*
     * $Name etc. reference.
     * 
     * In order to support expansion of lookup results, we must save the lookup
     * result. We use the input buffer since it will not be needed anymore.
     */
    if (type == MAC_PARSE_EXPR) {

	/*
	 * Look for the ? or : delimiter. In case of a syntax error, return
	 * without doing damage, and issue a warning instead.
	 */
	for (cp = acl_vstring_str(buf); /* void */ ; cp++) {
	    if ((ch = *cp) == 0) {
		lookup_mode = MAC_EXP_MODE_USE;
		break;
	    }
	    if (ch == '?' || ch == ':') {
		*cp++ = 0;
		lookup_mode = MAC_EXP_MODE_TEST;
		break;
	    }
	    if (!ACL_ISALNUM(ch) && ch != '_') {
		acl_msg_warn("macro name syntax error: \"%s\"", acl_vstring_str(buf));
		mc->status |= MAC_PARSE_ERROR;
		return (mc->status);
	    }
	}

	/*
	 * Look up the named parameter.
	 */
	text = mc->lookup(acl_vstring_str(buf), lookup_mode, mc->context, &pp, &size);

	/*
	 * Perform the requested substitution.
	 */
	switch (ch) {
	case '?':
	    if (text != 0 && *text != 0)
		mac_parse(cp, mac_expand_callback, mc);
	    break;
	case ':':
	    if (text == 0 || *text == 0)
		mac_parse(cp, mac_expand_callback, mc);
	    break;
	default:
	    if (text == 0) {
		mc->status |= MAC_PARSE_UNDEF;
	    } else if (*text == 0) {
		 /* void */ ;
	    } else if (mc->flags & MAC_EXP_FLAG_RECURSE) {
		acl_vstring_strcpy(buf, text);
		mac_parse(acl_vstring_str(buf), mac_expand_callback, mc);
	    } else {
		len = (int) ACL_VSTRING_LEN(mc->result);
		acl_vstring_strcat(mc->result, text);
		if (mc->filter) {
		    cp = acl_vstring_str(mc->result) + len;
		    while (*(cp += strspn(cp, mc->filter)))
			*cp++ = '_';
		}
	    }
	    break;
	}
    }

    /*
     * Literal text.
     */
    else {
	acl_vstring_strcat(mc->result, acl_vstring_str(buf));
    }

    mc->level--;
	if (pp)
		free(pp);

    return (mc->status);
}

/* mac_expand - expand $name instances */

int     mac_expand(ACL_VSTRING *result, const char *pattern, int flags,
		           const char *filter,
		           MAC_EXP_LOOKUP_FN lookup, char *context)
{
    MAC_EXP mc;
    int     status;

    /*
     * Bundle up the request and do the substitutions.
     */
    mc.result = result;
    mc.flags = flags;
    mc.filter = filter;
    mc.lookup = lookup;
    mc.context = context;
    mc.status = 0;
    mc.level = 0;
    ACL_VSTRING_RESET(result);
    status = mac_parse(pattern, mac_expand_callback, &mc);
    ACL_VSTRING_TERMINATE(result);

    return (status);
}

#ifdef TEST

 /*
  * This code certainly deserves a stand-alone test program.
  */
#include <stdlib.h>
#include <stringops.h>
#include <htable.h>
#include <vstream.h>
#include <vstring_vstream.h>

static const char *lookup(const char *name, int unused_mode, char *context)
{
    HTABLE *table = (HTABLE *) context;

    return (htable_find(table, name));
}

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *buf = vstring_alloc(100);
    VSTRING *result = vstring_alloc(100);
    char   *cp;
    char   *name;
    char   *value;
    HTABLE *table;
    int     stat;

    while (!vstream_feof(VSTREAM_IN)) {

	table = htable_create(0);

	/*
	 * Read a block of definitions, terminated with an empty line.
	 */
	while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	    vstream_printf("<< %s\n", vstring_str(buf));
	    vstream_fflush(VSTREAM_OUT);
	    if (VSTRING_LEN(buf) == 0)
		break;
	    cp = vstring_str(buf);
	    name = mystrtok(&cp, " \t\r\n=");
	    value = mystrtok(&cp, " \t\r\n=");
	    htable_enter(table, name, value ? mystrdup(value) : 0);
	}

	/*
	 * Read a block of patterns, terminated with an empty line or EOF.
	 */
	while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	    vstream_printf("<< %s\n", vstring_str(buf));
	    vstream_fflush(VSTREAM_OUT);
	    if (VSTRING_LEN(buf) == 0)
		break;
	    cp = vstring_str(buf);
	    VSTRING_RESET(result);
	    stat = mac_expand(result, vstring_str(buf), MAC_EXP_FLAG_NONE,
			      (char *) 0, lookup, (char *) table);
	    vstream_printf("stat=%d result=%s\n", stat, vstring_str(result));
	    vstream_fflush(VSTREAM_OUT);
	}
	htable_free(table, myfree);
	vstream_printf("\n");
    }

    /*
     * Clean up.
     */
    vstring_free(buf);
    vstring_free(result);
    exit(0);
}

#endif
