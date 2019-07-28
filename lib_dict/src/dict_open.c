#include "StdAfx.h"
#include "dict_db.h"
#include "dict_cdb.h"
#include "dict_tc.h"
#include <string.h>
#include <stdlib.h>
#include "debug_var.h"
#include "dict.h"

 /*
  * lookup table for available map types.
  */
typedef struct {
	char   *type;
	struct DICT *(*open) (const char *, int, int);
} DICT_OPEN_INFO;

static DICT_OPEN_INFO dict_open_info[] = {
#ifdef HAS_CDB
	{ DICT_TYPE_CDB, dict_cdb_open, },
#endif
#ifdef	HAS_TC
	{ DICT_TYPE_TC, dict_tc_open },
#endif
#ifdef SNAPSHOT
	{ DICT_TYPE_TCP, dict_tcp_open, },
#endif
#ifdef HAS_SDBM
	{ DICT_TYPE_SDBM, dict_sdbm_open, },
#endif
#ifdef HAS_DBM
	{ DICT_TYPE_DBM, dict_dbm_open, },
#endif
#ifdef HAS_BDB
	{ DICT_TYPE_HASH, dict_hash_open, },
	{ DICT_TYPE_BTREE, dict_btree_open, },
#endif
#ifdef HAS_NIS
	{ DICT_TYPE_NIS, dict_nis_open, },
#endif
#ifdef HAS_NISPLUS
	{ DICT_TYPE_NISPLUS, dict_nisplus_open, },
#endif
#ifdef HAS_NETINFO
	{ DICT_TYPE_NETINFO, dict_ni_open, },
#endif
#ifdef HAS_PCRE
	{ DICT_TYPE_PCRE, dict_pcre_open, },
#endif
#ifdef HAS_POSIX_REGEXP
	{ DICT_TYPE_REGEXP, dict_regexp_open, },
#endif
	{ NULL, NULL, },
};

static ACL_HTABLE *dict_open_hash;

/* dict_open_init - one-off initialization */

void dict_open_init(void)
{
	const char *myname = "dict_open_init";
	DICT_OPEN_INFO *dp;

	if (dict_open_hash != 0) {
		acl_msg_warn("%s: multiple initialization, return", myname);
		return;
	}
	dict_open_hash = acl_htable_create(10, 0);

	for (dp = dict_open_info; dp->type; dp++)
		acl_htable_enter(dict_open_hash, dp->type, (char *) dp);
}

/* dict_open - open dictionary */

DICT   *dict_open(const char *dict_spec, int open_flags, int dict_flags)
{
	const char *myname = "dict_open";
	char   *saved_dict_spec = acl_mystrdup(dict_spec);
	char   *dict_name;
	DICT   *dict;

	if ((dict_name = acl_split_at(saved_dict_spec, ':')) == 0)
		acl_msg_fatal("%s: open dictionary: expecting \"type:[path/]name\" form instead of \"%s\"",
			myname, dict_spec);
	dict = dict_open3(saved_dict_spec, dict_name, open_flags, dict_flags);
	acl_myfree(saved_dict_spec);
	return (dict);
}

/* dict_open3 - open dictionary */

DICT   *dict_open3(const char *dict_type, const char *dict_name,
		int open_flags, int dict_flags)
{
	const char *myname = "dict_open3";
	DICT_OPEN_INFO *dp;
	DICT   *dict;

	if (*dict_type == 0 || *dict_name == 0)
		acl_msg_fatal("%s: open dictionary: expecting \"type:name\" form instead of \"%s:%s\"",
			myname, dict_type, dict_name);
	if (dict_open_hash == 0)
		acl_msg_fatal("%s: dict_open_init should be called first", myname);
	if ((dp = (DICT_OPEN_INFO *) acl_htable_find(dict_open_hash, dict_type)) == 0)
		acl_msg_fatal("unsupported dictionary type: %s", dict_type);
	if ((dict = dp->open(dict_name, open_flags, dict_flags)) == 0)
		acl_msg_fatal("opening %s:%s %s", dict_type, dict_name, acl_last_serror());
	acl_debug(DEBUG_DICT_OPEN, 1)("%s: %s:%s", myname, dict_type, dict_name);
	return (dict);
}

/* dict_open_register - register dictionary type */

void    dict_open_register(const char *type,
		DICT *(*open_fn) (const char *, int, int))
{
	const char *myname = "dict_open_register";
	DICT_OPEN_INFO *dp;

	if (dict_open_hash == 0)
		acl_msg_fatal("%s: dict_open_init should be called first", myname);
	if (acl_htable_find(dict_open_hash, type))
		acl_msg_panic("%s: dictionary type exists: %s", myname, type);
	dp = (DICT_OPEN_INFO *) acl_mymalloc(sizeof(*dp));
	dp->type = acl_mystrdup(type);
	dp->open = open_fn;
	acl_htable_enter(dict_open_hash, dp->type, (char *) dp);
}

/* dict_sort_alpha_cpp - qsort() callback */

static int dict_sort_alpha_cpp(const void *a, const void *b)
{
	return (strcmp(((char **) a)[0], ((char **) b)[0]));
}

/* dict_mapnames - return an ARGV of available map_names */

ACL_ARGV   *dict_mapnames()
{
	const char *myname = "dict_mapnames";
	ACL_HTABLE_INFO **ht_info;
	ACL_HTABLE_INFO **ht;
	DICT_OPEN_INFO *dp;
	ACL_ARGV   *mapnames;

	if (dict_open_hash == 0)
		acl_msg_fatal("%s: dict_open_init should be called first", myname);
	mapnames = acl_argv_alloc(acl_htable_used(dict_open_hash) + 1);
	for (ht_info = ht = acl_htable_list(dict_open_hash); *ht; ht++) {
		dp = (DICT_OPEN_INFO *) ht[0]->value;
		acl_argv_add(mapnames, dp->type, ACL_ARGV_END);
	}
	qsort((void *) mapnames->argv, mapnames->argc, sizeof(mapnames->argv[0]),
			dict_sort_alpha_cpp);
	acl_myfree(ht_info);
	acl_argv_terminate(mapnames);
	return mapnames;
}
