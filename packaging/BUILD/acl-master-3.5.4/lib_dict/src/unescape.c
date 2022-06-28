#include "StdAfx.h"
#include <ctype.h>
#include "unescape.h"

/* unescape - process escape sequences */

ACL_VSTRING *unescape(ACL_VSTRING *result, const char *data)
{
	int     ch;
	int     oval;
	int     i;

#define UCHAR(cp)	((const unsigned char *) (cp))
#define ISOCTAL(ch)	(ACL_ISDIGIT(ch) && (ch) != '8' && (ch) != '9')

	ACL_VSTRING_RESET(result);

	while ((ch = *UCHAR(data++)) != 0) {
		if (ch == '\\') {
			if ((ch = *UCHAR(data++)) == 0)
				break;
			switch (ch) {
			case 'a':				/* \a -> audible bell */
				ch = '\a';
				break;
			case 'b':				/* \b -> backspace */
				ch = '\b';
				break;
			case 'f':				/* \f -> formfeed */
				ch = '\f';
				break;
			case 'n':				/* \n -> newline */
				ch = '\n';
				break;
			case 'r':				/* \r -> carriagereturn */
				ch = '\r';
				break;
			case 't':				/* \t -> horizontal tab */
				ch = '\t';
				break;
			case 'v':				/* \v -> vertical tab */
				ch = '\v';
				break;
			case '0':				/* \nnn -> ASCII value */
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				for (oval = ch - '0', i = 0;
					i < 2 && (ch = *UCHAR(data)) != 0 && ISOCTAL(ch);
					i++, data++) {
					oval = (oval << 3) | (ch - '0');
				}
				ch = oval;
				break;
			default:				/* \any -> any */
				break;
			}
		}
		ACL_VSTRING_ADDCH(result, ch);
	}
	ACL_VSTRING_TERMINATE(result);
	return (result);
}

/* escape - reverse transformation */

ACL_VSTRING *escape(ACL_VSTRING *result, const char *data, ssize_t len)
{
	int     ch;

	ACL_VSTRING_RESET(result);
	while (len-- > 0) {
		ch = *UCHAR(data++);
		if (ACL_ISASCII(ch)) {
			if (ACL_ISPRINT(ch)) {
				if (ch == '\\')
					ACL_VSTRING_ADDCH(result, ch);
				ACL_VSTRING_ADDCH(result, ch);
				continue;
			} else if (ch == '\a') {		/* \a -> audible bell */
				acl_vstring_strcat(result, "\\a");
				continue;
			} else if (ch == '\b') {		/* \b -> backspace */
				acl_vstring_strcat(result, "\\b");
				continue;
			} else if (ch == '\f') {		/* \f -> formfeed */
				acl_vstring_strcat(result, "\\f");
				continue;
			} else if (ch == '\n') {		/* \n -> newline */
				acl_vstring_strcat(result, "\\n");
				continue;
			} else if (ch == '\r') {		/* \r -> carriagereturn */
				acl_vstring_strcat(result, "\\r");
				continue;
			} else if (ch == '\t') {		/* \t -> horizontal tab */
				acl_vstring_strcat(result, "\\t");
				continue;
			} else if (ch == '\v') {		/* \v -> vertical tab */
				acl_vstring_strcat(result, "\\v");
				continue;
			}
		}
		if (ACL_ISDIGIT(*UCHAR(data)))
			acl_vstring_sprintf_append(result, "\\%03d", ch);
		else
			acl_vstring_sprintf_append(result, "\\%d", ch);
	}
	ACL_VSTRING_TERMINATE(result);
	return (result);
}

#ifdef TEST

#include <stdlib.h>
#include <string.h>
#include <msg.h>
#include <vstring_vstream.h>

int     main(int argc, char **argv)
{
	VSTRING *in = vstring_alloc(10);
	VSTRING *out = vstring_alloc(10);
	int     un_escape = 1;

	if (argc > 2 || (argc > 1 && (un_escape = strcmp(argv[1], "-e"))) != 0)
		msg_fatal("usage: %s [-e (escape)]", argv[0]);

	if (un_escape) {
		while (vstring_fgets_nonl(in, VSTREAM_IN)) {
			unescape(out, vstring_str(in));
			vstream_fwrite(VSTREAM_OUT, vstring_str(out), VSTRING_LEN(out));
		}
	} else {
		while (vstring_fgets(in, VSTREAM_IN)) {
			escape(out, vstring_str(in), VSTRING_LEN(in));
			vstream_fwrite(VSTREAM_OUT, vstring_str(out), VSTRING_LEN(out));
		}
	}
	vstream_fflush(VSTREAM_OUT);
	exit(0);
}

#endif
