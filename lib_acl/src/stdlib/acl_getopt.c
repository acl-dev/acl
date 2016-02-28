#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_getopt.h"

char *acl_optarg = NULL;	/* Global argument pointer. */
int   acl_optind = 0;		/* Global argv index. */

static char *scan = NULL;	/* Private scan pointer. */

void acl_getopt_init(void)
{
	scan = NULL;
}

int acl_getopt(int argc, char * argv[], const char *optstring)
{
	register char c;
	register char *place;

	acl_optarg = NULL;

	if (scan == NULL || *scan == '\0') {
		if (acl_optind == 0)
			acl_optind++;
	
		if (acl_optind >= argc || argv[acl_optind][0] != '-' || argv[acl_optind][1] == '\0')
			return(EOF);
		if (strcmp(argv[acl_optind], "--")==0) {
			acl_optind++;
			return(EOF);
		}
	
		scan = argv[acl_optind]+1;
		acl_optind++;
	}

	c = *scan++;
	place = strchr(optstring, c);

	if (place == NULL || c == ':') {
		printf("%s: unknown option -%c\n", argv[0], c);
		return('?');
	}

	place++;
	if (*place == ':') {
		if (*scan != '\0') {
			acl_optarg = scan;
			scan = NULL;
		} else if (acl_optind < argc) {
			acl_optarg = argv[acl_optind];
			acl_optind++;
		} else {
			printf("%s: -%c argument missing\n", argv[0], c);
			return('?');
		}
	}

	return(c);
}
