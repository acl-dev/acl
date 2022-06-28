#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

typedef struct {
	char  name[256];
	int   ncalloc;
	int   nmalloc;
	int   nstrdup;
	int   nfree;
} FNAME;

static ACL_HTABLE *__table = NULL;

static FNAME *fname_new(const char *name)
{
	FNAME *fname = (FNAME*) acl_mycalloc(1, sizeof(FNAME));

	ACL_SAFE_STRNCPY(fname->name, name, sizeof(fname->name));
	return (fname);
}

static void fname_free(FNAME *fname)
{
	acl_myfree(fname);
}

static void run(const char *path)
{
	ACL_FILE *fp = acl_fopen(path, "r");
	FNAME *fname;
	char  buf[4096];
	ACL_ITER iter;
	int   nfree = 0, nmalloc = 0, ncalloc = 0, nstrdup = 0;

	if (fp == NULL) {
		printf("open file(%s) error(%s)\n", path, acl_last_serror());
		return;
	}

	__table = acl_htable_create(100, 0);

	while (acl_fgets_nonl(buf, sizeof(buf), fp) != NULL) {
		ACL_ARGV *argv = acl_argv_split(buf, " ");
		const char *oper, *name;

		if (argv->argc != 8) {
			acl_argv_free(argv);
			continue;
		}
		oper = argv->argv[5];
		name = argv->argv[6];
		if (strcmp(oper, "free:") == 0) {
			fname = (FNAME*) acl_htable_find(__table, name);
			if (fname == NULL) {
				fname = fname_new(name);
				acl_htable_enter(__table, name, fname);
			}
			fname->nfree++;
		} else if (strcmp(oper, "malloc:") == 0) {
			fname = (FNAME*) acl_htable_find(__table, name);
			if (fname == NULL) {
				fname = fname_new(name);
				acl_htable_enter(__table, name, fname);
			}
			fname->nmalloc++;
		} else if (strcmp(oper, "calloc:") == 0) {
			fname = (FNAME*) acl_htable_find(__table, name);
			if (fname == NULL) {
				fname = fname_new(name);
				acl_htable_enter(__table, name, fname);
			}
			fname->ncalloc++;
		} else if (strcmp(oper, "strdup:") == 0) {
			fname = (FNAME*) acl_htable_find(__table, name);
			if (fname == NULL) {
				fname = fname_new(name);
				acl_htable_enter(__table, name, fname);
			}
			fname->nstrdup++;
		}
		acl_argv_free(argv);
	}

	acl_foreach(iter, __table) {
		fname = (FNAME*) iter.data;
		nmalloc += fname->nmalloc;
		ncalloc += fname->ncalloc;
		nstrdup += fname->nstrdup;
		nfree += fname->nfree;
		if (fname->nmalloc + fname->ncalloc + fname->nstrdup == fname->nfree)
			continue;
		printf(">>>%s: total free: %d, total alloc: %d (malloc: %d, calloc: %d, strdup: %d)\n",
			fname->name, fname->nfree, fname->nmalloc + fname->ncalloc + fname->nstrdup,
			fname->nmalloc, fname->ncalloc, fname->nstrdup);
	}

	printf(">>> total free: %d, total alloc: %d (malloc: %d, calloc: %d, strdup: %d)\n",
		nfree, nmalloc + ncalloc + nstrdup, nmalloc, ncalloc, nstrdup);

	acl_htable_free(__table, (void (*)(void*)) fname_free);
	acl_fclose(fp);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -f log_path\n", procname);
}

int main(int argc, char *argv[])
{
	char  ch, path[256] = "";

	while ((ch = getopt(argc, argv, "hf:")) > 0) {
		switch (ch) {
		case 'f':
			snprintf(path, sizeof(path), "%s", optarg);
			break;
		case 'h':
		default:
			usage(argv[0]);
			return (0);
		}
	}

	if (path[0] == 0) {
		usage(argv[0]);
		return (0);
	}
	
	run(path);

	return (0);
}
