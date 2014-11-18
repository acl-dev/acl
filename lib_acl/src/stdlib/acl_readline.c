#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <ctype.h>
#include <stdlib.h>
#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_readline.h"

#endif

#define STR(x) acl_vstring_str(x)
#define LEN(x) ACL_VSTRING_LEN(x)
#define END(x) acl_vstring_end(x)

/* readlline - read one logical line */

ACL_VSTRING *acl_readlline(ACL_VSTRING *buf, ACL_VSTREAM *fp, int *lineno)
{
	char  *cp;
	size_t start;
	int   ch, next;

	ACL_VSTRING_RESET(buf);

	/*
	 * Ignore comment lines, all whitespace lines, and empty lines. Terminate
	 * at EOF or at the beginning of the next logical line.
	 */
	for (;;) {
		/* Read one line, possibly not newline terminated. */
		start = LEN(buf);
		while ((ch = acl_vstream_getc(fp)) != ACL_VSTREAM_EOF && ch != '\n')
			ACL_VSTRING_ADDCH(buf, ch);
		if (ch == '\n' && lineno != 0)
			*lineno += 1;
		/* Ignore comment line, all whitespace line, or empty line. */
		for (cp = STR(buf) + start; cp < END(buf) && ACL_ISSPACE(*cp); cp++)
			/* void */ ;
		if (cp == END(buf) || *cp == '#')
			acl_vstring_truncate(buf, start);
		/* Terminate at EOF or at the beginning of the next logical line. */
		if (ch == ACL_VSTREAM_EOF)
			break;
		if (LEN(buf) > 0) {
			if ((next = acl_vstream_getc(fp)) != ACL_VSTREAM_EOF)
				acl_vstream_ungetc(fp, next);
			if (next != '#' && !ACL_ISSPACE(next))
				break;
		}
	}
	ACL_VSTRING_TERMINATE(buf);

	/*
	 * Invalid input: continuing text without preceding text. Allowing this
	 * would complicate "postconf -e", which implements its own multi-line
	 * parsing routine. Do not abort, just warn, so that critical programs
	 * like postmap do not leave behind a truncated table.
	 */
	if (LEN(buf) > 0 && ACL_ISSPACE(*STR(buf))) {
		acl_msg_warn("%s: logical line must not start with whitespace: \"%.30s%s\"",
			ACL_VSTREAM_PATH(fp), STR(buf), LEN(buf) > 30 ? "..." : "");
		return (acl_readlline(buf, fp, lineno));
	}

	/*
	 * Done.
	 */
	return (LEN(buf) > 0 ? buf : NULL);
}

