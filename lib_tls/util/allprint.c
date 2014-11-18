#include "StdAfx.h"
#include "stringops.h"

/* allprint - return true if string is all printable */

int     allprint(const char *string)
{
    const char *cp;
    int     ch;

    if (*string == 0)
	return (0);
    for (cp = string; (ch = *(const unsigned char *) cp) != 0; cp++)
	if (!ACL_ISASCII(ch) || !ACL_ISPRINT(ch))
	    return (0);
    return (1);
}
