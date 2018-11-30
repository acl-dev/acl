#include "stdafx.h"
#include "strops.h"

/* mystrtok - safe tokenizer */

char *mystrtok(char **src, const char *sep)
{
	char   *start = *src;
	char   *end;

	/*
	 * Skip over leading delimiters.
	 */
	start += strspn(start, sep);
	if (*start == 0) {
		*src = start;
		return 0;
	}

	/*
	 * Separate off one token.
	 */
	end = start + strcspn(start, sep);
	if (*end != 0)
		*end++ = 0;
	*src = end;
	return start;
}

char *lowercase(char *s)
{
	char *cp = s;

	if (s == NULL)
		return NULL;

	while (*cp) {
		*cp = tolower(*cp);
		cp++;
	}

	return s;
}

int alldig(const char *s)
{
	if (s == NULL || *s == 0)
		return 0;
	for (; *s != 0; s++)
		if (!isdigit(*s))
			return 0;
	return 1;
}
