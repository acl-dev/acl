#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_debug.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"

#endif

#define	DEF_DEBUG_SECTIONS	1000
static int __max_sections = DEF_DEBUG_SECTIONS;

static int *__debug_levels = NULL;

void acl_debug_end()
{
	if (__debug_levels != NULL) {
		acl_myfree(__debug_levels);
		__debug_levels = NULL;
	}
}

/* ptr format: 1:1; 2:10; 3:8... */
void acl_debug_init(const char *ptr)
{
	acl_debug_init2(ptr, DEF_DEBUG_SECTIONS);
}

void acl_debug_init2(const char *str, int max_debug_level)
{
	int i, level_all = -1, section, level;
	char *buf = acl_mystrdup(str), *tmp;
	ACL_ARGV *tokens;

	tmp = acl_mystr_trim(buf);
	if (*tmp == 0) {
		acl_myfree(buf);
		return;
	}
	tokens = acl_argv_split(tmp, ";,");
	acl_myfree(buf);

	__max_sections = max_debug_level > 100 ? max_debug_level : 100;

	if (__debug_levels)
		acl_myfree(__debug_levels);

	__debug_levels = (int*) acl_mycalloc(__max_sections, sizeof(int));

	for (i = 0; i < __max_sections; i++)
		__debug_levels[i] = 0;

	for (i = 0; i < tokens->argc; i++) {
		size_t n;
		char* ptr = tokens->argv[i];
		ACL_ARGV* pair = acl_argv_split(ptr, ":");
		if (pair->argc != 2) {
			acl_argv_free(pair);
			continue;
		}

		if (strcasecmp(pair->argv[0], "All") == 0) {
			level_all = atoi(pair->argv[1]);
			acl_argv_free(pair);
			continue;
		}

		ptr = pair->argv[0];
		n = strcspn(ptr, "->|,;.@{}[]<>#$%^&()+*!");
		ptr[n] = 0;

		section = atoi(pair->argv[0]);
		level = atoi(pair->argv[1]);

		acl_argv_free(pair);

		if (section < __max_sections && section >= 0 && level >= 0)
			__debug_levels[section] = level;
	}

	acl_argv_free(tokens);

	if (level_all >= 0) {
		for (i = 0; i < __max_sections; i++) {
			if (__debug_levels[i] < level_all)
				__debug_levels[i] = level_all;
		}
	}
}

int acl_do_debug(int section, int level)
{
	if (__debug_levels == NULL)
		return (0);
	if (section >= __max_sections || section < 0)
		return (0);
	if (level > __debug_levels[section])
		return (0);

	return (1);
}
