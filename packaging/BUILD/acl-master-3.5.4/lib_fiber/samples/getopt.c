
/*
Newsgroups: mod.std.unix
Subject: public domain AT&T getopt source
Date: 3 Nov 85 19:34:15 GMT

Here's something you've all been waiting for:  the AT&T public domain
source for getopt(3).  It is the code which was given out at the 1985
UNIFORUM conference in Dallas.  I obtained it by electronic mail
directly from AT&T.  The people there assure me that it is indeed
in the public domain.
*/

/*LINTLIBRARY*/
#include <stdio.h>
#include <stdlib.h>
#include "patch.h"

#ifdef	HAVE_NO_GETOPT

#define EOF		(-1)
#define ERR(s, c) \
	if (opterr) { \
		char errbuf[2]; \
		errbuf[0] = c; errbuf[1] = '\n'; \
		(void) printf("%s", argv[0]); \
		(void) printf("%s", s); \
		(void) printf("%s", errbuf); \
	}


int   opterr;
int   optind;
int   optopt;
char *optarg;

int getopt(int argc, char * const argv[], char *opts)
{
	static int sp = 1;
	register int c;
	register char *cp;
	static int call_first = 1;

	if (call_first) {
		opterr = 1;
		optind = 1;
		call_first = 0;
	}

	if(sp == 1)
		if(optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == 0) {
			optind++;
			return(EOF);
		}
	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=strchr(opts, c)) == NULL) {
		ERR(": illegal option -- ", c);
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			ERR(": option requires an argument -- ", c);
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}
#endif	/* HAVE_NO_GETOPT */
