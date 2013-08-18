#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_debug.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"

#endif

#define	DEF_DEBUG_SECTIONS	1000
static int __max_debug_sections = DEF_DEBUG_SECTIONS;

static int *pDebugLevels = NULL;

void acl_debug_end()
{
	if (pDebugLevels != NULL) {
		acl_myfree(pDebugLevels);
		pDebugLevels = NULL;
	}
}

/* pStr format: 1,1; 2,10; 3,8... */
void acl_debug_init(const char *pStr)
{
	acl_debug_init2(pStr, DEF_DEBUG_SECTIONS);
}

void acl_debug_init2(const char *pStr, int max_debug_level)
{
	int i, level_all = -1, section, level;
	ACL_ARGV* pItem = acl_argv_split(pStr, ";");

	__max_debug_sections = max_debug_level > 100 ? max_debug_level : 100;

	pDebugLevels = (int*) acl_mycalloc(__max_debug_sections, sizeof(int));

	for (i = 0; i < __max_debug_sections; i++)
		pDebugLevels[i] = 0;

	for (i = 0; i < pItem->argc; i++) {
		char* pSection = pItem->argv[i];
		ACL_ARGV* pPair = acl_argv_split(pSection, ":,\t ");
		if (pPair->argc != 2) {
			acl_argv_free(pPair);
			continue;
		}

		if (strcasecmp(pPair->argv[0], "All") == 0) {
			level_all = atoi(pPair->argv[1]);
			acl_argv_free(pPair);
			continue;
		}

		section = atoi(pPair->argv[0]);
		level = atoi(pPair->argv[1]);

		acl_argv_free(pPair);

		if (section >= __max_debug_sections || section < 0 || level < 0) {
			continue;
		}
		pDebugLevels[section] = level;
	}

	acl_argv_free(pItem);

	if (level_all >= 0) {
		for (i = 0; i < __max_debug_sections; i++) {
			if (pDebugLevels[i] < level_all)
				pDebugLevels[i] = level_all;
		}
	}
}

int acl_do_debug(int section, int level)
{
	if (pDebugLevels == NULL)
		return (0);
	if (section >= __max_debug_sections || section < 0)
		return (0);
	if (level > pDebugLevels[section])
		return (0);

	return (1);
}
