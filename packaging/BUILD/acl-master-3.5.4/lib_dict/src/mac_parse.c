#include "StdAfx.h"
#include "debug_var.h"
#include <ctype.h>
#include "mac_parse.h"

 /*
  * Helper macro for consistency. Null-terminate the temporary buffer,
  * execute the action, and reset the temporary buffer for re-use.
  */
#define MAC_PARSE_ACTION(status, type, buf, context) \
	do { \
	    ACL_VSTRING_TERMINATE(buf); \
	    status |= action((type), (buf), (context)); \
	    ACL_VSTRING_RESET(buf); \
	} while(0)

/* mac_parse - split string into literal text and macro references */

int     mac_parse(const char *value, MAC_PARSE_FN action, void *context)
{
    const char *myname = "mac_parse";
    ACL_VSTRING *buf = acl_vstring_alloc(1);	/* result buffer */
    const char *vp;			/* value pointer */
    const char *pp;			/* open_paren pointer */
    const char *ep;			/* string end pointer */
    static char open_paren[] = "({";
    static char close_paren[] = ")}";
    int     level;
    int     status = 0;

#define SKIP(start, var, cond) \
        for (var = start; *var && (cond); var++);

    acl_debug(DEBUG_MAC, 2) ("%s: %s", myname, value);

    for (vp = value; *vp;) {
	if (*vp != '$') {			/* ordinary character */
	    ACL_VSTRING_ADDCH(buf, *vp);
	    vp += 1;
	} else if (vp[1] == '$') {		/* $$ becomes $ */
	    ACL_VSTRING_ADDCH(buf, *vp);
	    vp += 2;
	} else {				/* found bare $ */
	    if (ACL_VSTRING_LEN(buf) > 0)
		MAC_PARSE_ACTION(status, MAC_PARSE_LITERAL, buf, context);
	    vp += 1;
	    pp = open_paren;
	    if (*vp == *pp || *vp == *++pp) {	/* ${x} or $(x) */
		level = 1;
		vp += 1;
		for (ep = vp; level > 0; ep++) {
		    if (*ep == 0) {
			acl_msg_warn("truncated macro reference: \"%s\"", value);
			status |= MAC_PARSE_ERROR;
			break;
		    }
		    if (*ep == *pp)
			level++;
		    if (*ep == close_paren[pp - open_paren])
			level--;
		}
		if (status & MAC_PARSE_ERROR)
		    break;
		acl_vstring_strncat(buf, vp, level > 0 ? ep - vp : ep - vp - 1);
		vp = ep;
	    } else {				/* plain $x */
		SKIP(vp, ep, ACL_ISALNUM(*ep) || *ep == '_');
		acl_vstring_strncat(buf, vp, ep - vp);
		vp = ep;
	    }
	    if (ACL_VSTRING_LEN(buf) == 0) {
		status |= MAC_PARSE_ERROR;
		acl_msg_warn("empty macro name: \"%s\"", value);
		break;
	    }
	    MAC_PARSE_ACTION(status, MAC_PARSE_EXPR, buf, context);
	}
    }
    if (ACL_VSTRING_LEN(buf) > 0 && (status & MAC_PARSE_ERROR) == 0)
	MAC_PARSE_ACTION(status, MAC_PARSE_LITERAL, buf, context);

    /*
     * Cleanup.
     */
    acl_vstring_free(buf);

    return (status);
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Read strings from stdin, print parsed
  * result to stdout.
  */
#include <vstring_vstream.h>

/* mac_parse_print - print parse tree */

static int mac_parse_print(int type, VSTRING *buf, char *unused_context)
{
    char   *type_name;

    switch (type) {
    case MAC_PARSE_EXPR:
	type_name = "MAC_PARSE_EXPR";
	break;
    case MAC_PARSE_LITERAL:
	type_name = "MAC_PARSE_LITERAL";
	break;
    default:
	msg_panic("unknown token type %d", type);
    }
    vstream_printf("%s \"%s\"\n", type_name, vstring_str(buf));
    return (0);
}

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *buf = vstring_alloc(1);

    while (vstring_fgets_nonl(buf, VSTREAM_IN)) {
	mac_parse(vstring_str(buf), mac_parse_print, (char *) 0);
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(buf);
    return (0);
}

#endif
