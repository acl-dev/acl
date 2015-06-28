#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>

/* Utility library. */
#include "thread/acl_pthread.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_sane_basename.h"

#endif

#define STR(x)	acl_vstring_str(x)

/* acl_sane_basename - skip directory prefix */

char *acl_sane_basename(ACL_VSTRING *bp, const char *path)
{
	const char *myname = "acl_sane_basename";
	const char *first;
	const char *last;

	/*
	 * Your buffer or mine?
	 */
	if (bp == 0) {
		acl_msg_error("%s(%d): bp null", myname, __LINE__);
		return (NULL);
	}

	/*
	 * Special case: return "." for null or zero-length input.
	 */
	if (path == 0 || *path == 0)
		return (STR(acl_vstring_strcpy(bp, ".")));

	/*
	 * Remove trailing '/' characters from input. Return "/" if input is all
	 * '/' characters.
	 */
	last = path + strlen(path) - 1;
#ifdef	ACL_UNIX
	while (*last == '/') {
#elif	defined(ACL_WINDOWS)
	while (*last == '/' || *last == '\\') {
#endif
		if (last == path)
			return (STR(acl_vstring_strcpy(bp, "/")));
		last--;
	}

	/*
	 * The pathname does not end in '/'. Skip to last '/' character if any.
	 */
	first = last - 1;
#ifdef	ACL_UNIX
	while (first >= path && *first != '/')
#elif	defined(ACL_WINDOWS)
	while (first >= path && *first != '/' && *first != '\\')
#endif
		first--;

	return (STR(acl_vstring_strncpy(bp, first + 1, last - first)));
}

/* acl_sane_dirname - keep directory prefix */

char *acl_sane_dirname(ACL_VSTRING *bp, const char *path)
{
	const char *myname = "acl_sane_dirname";
	const char *last;

	/*
	 * Your buffer or mine?
	 */
	if (bp == 0) {
		acl_msg_error("%s(%d): bp null", myname, __LINE__);
		return (NULL);
	}

	/*
	 * Special case: return "." for null or zero-length input.
	 */
	if (path == 0 || *path == 0)
		return (STR(acl_vstring_strcpy(bp, ".")));

	/*
	 * Remove trailing '/' characters from input. Return "/" if input is all
	 * '/' characters.
	 */
	last = path + strlen(path) - 1;
#ifdef	ACL_UNIX
	while (*last == '/') {
#elif	defined(ACL_WINDOWS)
	while (*last == '/' || *last == '\\') {
#endif
		if (last == path)
			return (STR(acl_vstring_strcpy(bp, "/")));
		last--;
	}

	/*
	 * This pathname does not end in '/'. Skip to last '/' character if any.
	 */
#ifdef	ACL_UNIX
	while (last >= path && *last != '/')
#elif	defined(ACL_WINDOWS)
	while (last >= path && *last != '/' && *last != '\\')
#endif
		last--;
	if (last < path)				/* no '/' */
		return (STR(acl_vstring_strcpy(bp, ".")));

	/*
	 * Strip trailing '/' characters from dirname (not strictly needed).
	 */
#ifdef	ACL_UNIX
	while (last > path && *last == '/')
#elif	defined(ACL_WINDOWS)
	while (last > path && (*last == '/' || *last == '\\'))
#endif
		last--;

	return (STR(acl_vstring_strncpy(bp, path, last - path + 1)));
}

#ifdef TEST
#include "stdlib/acl_vstring_vstream.h"

int     main(int argc acl_unused, char **argv acl_unused)
{
	ACL_VSTRING *buf = acl_vstring_alloc(10);
	char   *dir;
	char   *base;

	while (acl_vstring_gets_nonl(buf, ACL_VSTREAM_IN) > 0) {
		dir = acl_sane_dirname((ACL_VSTRING *) 0, STR(buf));
		base = acl_sane_basename((ACL_VSTRING *) 0, STR(buf));
		acl_vstream_printf("input=\"%s\" dir=\"%s\" base=\"%s\"\n",
				STR(buf), dir, base);
	}
	acl_vstream_fflush(ACL_VSTREAM_OUT);
	acl_vstring_free(buf);
	return (0);
}

#endif

