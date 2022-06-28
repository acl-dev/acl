#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_vstring_vstream.h"

#endif

/*
 * Macro to return the last character added to a ACL_VSTRING, for consistency.
 */
#define ACL_VSTRING_GET_RESULT(vp) \
	(ACL_VSTRING_LEN(vp) > 0 ? acl_vstring_end(vp)[-1] : ACL_VSTREAM_EOF)

/* acl_vstring_gets - read line from file, keep newline */

int acl_vstring_gets(ACL_VSTRING *vp, ACL_VSTREAM *fp)
{
	int     ch;

	ACL_VSTRING_RESET(vp);
	while ((ch = ACL_VSTREAM_GETC(fp)) != ACL_VSTREAM_EOF) {
		ACL_VSTRING_ADDCH(vp, ch);
		if (ch == '\n')
			break;
	}
	ACL_VSTRING_TERMINATE(vp);
	return (ACL_VSTRING_GET_RESULT(vp));
}

/* acl_vstring_gets_nonl - read line from file, strip newline */

int acl_vstring_gets_nonl(ACL_VSTRING *vp, ACL_VSTREAM *fp)
{
	int     ch, last = 0;

	ACL_VSTRING_RESET(vp);
	while ((ch = ACL_VSTREAM_GETC(fp)) != ACL_VSTREAM_EOF && ch != '\n') {
		if (ch == '\r') {
			last = ch;
		} else {
			if (last != 0) {
				ACL_VSTRING_ADDCH(vp, last);
				last = 0;
			}
			ACL_VSTRING_ADDCH(vp, ch);
		}
	}
	ACL_VSTRING_TERMINATE(vp);
	return (ch == '\n' ? ch : ACL_VSTRING_GET_RESULT(vp));
}

/* acl_vstring_gets_null - read null-terminated string from file */

int acl_vstring_gets_null(ACL_VSTRING *vp, ACL_VSTREAM *fp)
{
	int     ch;

	ACL_VSTRING_RESET(vp);
	while ((ch = ACL_VSTREAM_GETC(fp)) != ACL_VSTREAM_EOF && ch != 0)
		ACL_VSTRING_ADDCH(vp, ch);
	ACL_VSTRING_TERMINATE(vp);
	return (ch == 0 ? ch : ACL_VSTRING_GET_RESULT(vp));
}

/* acl_vstring_gets_bound - read line from file, keep newline, up to bound */

int acl_vstring_gets_bound(ACL_VSTRING *vp, ACL_VSTREAM *fp, ssize_t bound)
{
	const char *myname = "acl_vstring_gets_bound";
	int     ch;

	if (bound <= 0)
		acl_msg_panic("%s: invalid bound %ld", myname, (long) bound);

	ACL_VSTRING_RESET(vp);
	while (bound-- > 0 && (ch = ACL_VSTREAM_GETC(fp)) != ACL_VSTREAM_EOF) {
		ACL_VSTRING_ADDCH(vp, ch);
		if (ch == '\n')
			break;
	}
	ACL_VSTRING_TERMINATE(vp);
	return (ACL_VSTRING_GET_RESULT(vp));
}

/* acl_vstring_gets_nonl_bound - read line from file, strip newline, up to bound */

int acl_vstring_gets_nonl_bound(ACL_VSTRING *vp, ACL_VSTREAM *fp, ssize_t bound)
{
	const char *myname = "acl_vstring_gets_nonl_bound";
	int     ch = 0, last = 0;

	if (bound <= 0)
		acl_msg_panic("%s: invalid bound %ld", myname, (long) bound);

	ACL_VSTRING_RESET(vp);
	while (bound-- > 0 && (ch = ACL_VSTREAM_GETC(fp)) != ACL_VSTREAM_EOF && ch != '\n') {
		if (ch == '\r') {
			last = ch;
		} else {
			if (last != 0) {
				ACL_VSTRING_ADDCH(vp, last);
				if (bound-- == 0)
					break;
				last = 0;
			}
			ACL_VSTRING_ADDCH(vp, ch);
		}
	}
	ACL_VSTRING_TERMINATE(vp);
	return (ch == '\n' ? ch : ACL_VSTRING_GET_RESULT(vp));
}

/* acl_vstring_gets_null_bound - read null-terminated string from file */

int acl_vstring_gets_null_bound(ACL_VSTRING *vp, ACL_VSTREAM *fp, ssize_t bound)
{
	const char *myname = "acl_vstring_gets_null_bound";
	int     ch = ACL_VSTREAM_EOF;

	if (bound <= 0)
		acl_msg_panic("%s: invalid bound %ld", myname, (long) bound);

	ACL_VSTRING_RESET(vp);
	while (bound-- > 0 && (ch = ACL_VSTREAM_GETC(fp)) != ACL_VSTREAM_EOF && ch != 0)
		ACL_VSTRING_ADDCH(vp, ch);
	ACL_VSTRING_TERMINATE(vp);
	return (ch == 0 ? ch : ACL_VSTRING_GET_RESULT(vp));
}

