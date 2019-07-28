#include "StdAfx.h"
#include "stringops.h"

char   *printable(char *string, int replacement)
{
    char   *cp;
    int     ch;

    for (cp = string; (ch = *(unsigned char *) cp) != 0; cp++)
	if (!ACL_ISASCII(ch) || !ACL_ISPRINT(ch))
	    *cp = replacement;
    return (string);
}
