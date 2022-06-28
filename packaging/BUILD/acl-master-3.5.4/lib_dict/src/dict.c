#include "StdAfx.h"
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "debug_var.h"
#include "mac_expand.h"
#include "dict.h"
#include "name_mask.h"
#include "dict_ht.h"

/*
 * By default, use a sane default for an unknown name.
 */
int     dict_unknown_allowed = 1;
__thread int     dict_errno = 0;

static ACL_HTABLE *dict_table;

/*
 * Each (name, dictionary) instance has a reference count. The count is part
 * of the name, not the dictionary. The same dictionary may be registered
 * under multiple names. The structure below keeps track of instances and
 * reference counts.
 */
typedef struct {
	DICT   *dict;
	int     refcount;
} DICT_NODE;

#define dict_node(dict) \
	(dict_table ? (DICT_NODE *) acl_htable_find(dict_table, dict) : 0)

#define STR(x)	acl_vstring_str(x)

void    dict_init()
{
	const char *myname = "dict_init";

	if (dict_table != 0) {
		acl_msg_warn("%s: multiple initialization, return", myname);
		return;
	}

	dict_open_init();
	dict_table = acl_htable_create(0, 0);
}

/* dict_register - make association with dictionary */

void    dict_register(const char *dict_name, DICT *dict_info)
{
	const char *myname = "dict_register";
	DICT_NODE *node;

	if (dict_table == 0)
		acl_msg_fatal("%s(%d): call dict_init first", myname, __LINE__);
	if ((node = dict_node(dict_name)) == 0) {
		node = (DICT_NODE *) acl_mymalloc(sizeof(*node));
		node->dict = dict_info;
		node->refcount = 0;
		acl_htable_enter(dict_table, dict_name, (char *) node);
	} else if (dict_info != node->dict)
		acl_msg_fatal("%s: dictionary name exists: %s", myname, dict_name);
	node->refcount++;
	acl_debug(DEBUG_DICT, 2) ("%s: %s %d", myname, dict_name, node->refcount);
}

/* dict_handle - locate generic dictionary handle */

DICT   *dict_handle(const char *dict_name)
{
	DICT_NODE *node;

	return (node = dict_node(dict_name)) != 0 ? node->dict : 0;
}

/* dict_node_free - dict_unregister() callback */

static void dict_node_free(void *ptr)
{
	DICT_NODE *node = (DICT_NODE *) ptr;
	DICT   *dict = node->dict;

	if (dict->close)
		dict->close(dict);
	acl_myfree(node);
}

/* dict_unregister - break association with named dictionary */

void    dict_unregister(const char *dict_name)
{
	const char *myname = "dict_unregister";
	DICT_NODE *node;

	if (dict_table == 0)
		acl_msg_fatal("%s(%d): call dict_init first", myname, __LINE__);
	if ((node = dict_node(dict_name)) == 0)
		acl_msg_panic("non-existing dictionary: %s", dict_name);
	acl_debug(DEBUG_DICT, 2)("%s: %s %d", myname, dict_name, node->refcount);
	if (--(node->refcount) == 0)
		acl_htable_delete(dict_table, dict_name, dict_node_free);
}

/* dict_update - replace or add dictionary entry */

void    dict_update(const char *dict_name, char *key, char *value, size_t len)
{
	const char *myname = "dict_update";
	DICT_NODE *node;
	DICT   *dict;

	if (dict_table == 0)
		acl_msg_fatal("%s(%d): call dict_init first", myname, __LINE__);
	if ((node = dict_node(dict_name)) == 0) {
		if (dict_unknown_allowed == 0)
			acl_msg_fatal("%s: unknown dictionary: %s",
				myname, dict_name);
		dict = dict_ht_open(dict_name,
				acl_htable_create(0, 0),
				(void (*)(void*)) acl_myfree_fn);
		dict_register(dict_name, dict);
	} else
		dict = node->dict;
	acl_debug(DEBUG_DICT, 2)("%s: %s = %s", myname, key, value);
	dict->update(dict, key, strlen(key), value, len);
}

/* dict_lookup - look up dictionary entry */

const char *dict_lookup(const char *dict_name, char *key,
			char **value, size_t *size)
{
	const char *myname = "dict_lookup";
	DICT_NODE *node;
	DICT   *dict;
	const char *ret = 0;

	if (dict_table == 0)
		acl_msg_fatal("%s(%d): call dict_init first", myname, __LINE__);
	if ((node = dict_node(dict_name)) == 0) {
		if (dict_unknown_allowed == 0)
			acl_msg_fatal("%s: unknown dictionary: %s",
				myname, dict_name);
	} else {
		dict = node->dict;
		ret = dict->lookup(dict, key, strlen(key), value, size);
		if (ret == 0 && dict_unknown_allowed == 0)
			acl_msg_fatal("dictionary %s: unknown member: %s",
				dict_name, key);
	}
	acl_debug(DEBUG_DICT, 2)("%s: %s = %s", myname, key,
		ret ? ret : "(not found)");
	return ret;
}

/* dict_delete - delete dictionary entry */

int     dict_delete(const char *dict_name, char *key)
{
	const char *myname = "dict_delete";
	DICT_NODE *node;
	DICT   *dict;
	int     result;

	if (dict_table == 0)
		acl_msg_fatal("%s(%d): call dict_init first", myname, __LINE__);
	if ((node = dict_node(dict_name)) == 0) {
		if (dict_unknown_allowed == 0)
			acl_msg_fatal("%s: unknown dictionary: %s",
				myname, dict_name);
		dict = dict_ht_open(dict_name,
				acl_htable_create(0, 0),
				(void (*)(void*)) acl_myfree_fn);
		dict_register(dict_name, dict);
	} else
		dict = node->dict;
	acl_debug(DEBUG_DICT, 2)("%s: delete %s", myname, key);
	if ((result = dict->delete_it(dict, key, strlen(key))) != 0
			&& dict_unknown_allowed == 0)
		acl_msg_fatal("%s: dictionary %s: unknown member: %s",
			myname, dict_name, key);
	return result;
}

/* dict_sequence_reset - close the sequence cursor */

void dict_sequence_reset(const char *dict_name)
{
	const char *myname = "dict_sequence_reset";
	DICT_NODE *node;
	DICT   *dict;

	if (dict_table == 0)
		acl_msg_fatal("%s(%d): call dict_init first", myname, __LINE__);
	if ((node = dict_node(dict_name)) == 0) {
		if (dict_unknown_allowed == 0)
			acl_msg_fatal("%s: unknown dictionary: %s",
				myname, dict_name);
		dict = dict_ht_open(dict_name, acl_htable_create(0, 0),
				(void (*)(void*)) acl_myfree_fn);
		dict_register(dict_name, dict);
	} else
		dict = node->dict;

	if (dict->sequence_reset != NULL)
		dict->sequence_reset(dict);
}

/* dict_sequence_delcur - delete the data refered by the cursor */

int dict_sequence_delcur(const char *dict_name)
{
	const char *myname = "dict_sequence_delcur";
	DICT_NODE *node;
	DICT   *dict;

	if (dict_table == 0)
		acl_msg_fatal("%s(%d): call dict_init first", myname, __LINE__);
	if ((node = dict_node(dict_name)) == 0) {
		if (dict_unknown_allowed == 0)
			acl_msg_fatal("%s: unknown dictionary: %s",
				myname, dict_name);
		dict = dict_ht_open(dict_name, acl_htable_create(0, 0),
				(void (*)(void*)) acl_myfree_fn);
		dict_register(dict_name, dict);
	} else
		dict = node->dict;

	if (dict->sequence_delcur != NULL)
		return dict->sequence_delcur(dict);

	return -1;
}

/* dict_sequence - traverse dictionary */

int     dict_sequence(const char *dict_name, const int func,
	char **key, size_t *key_size, char **value, size_t *value_size)
{
	const char *myname = "dict_sequence";
	DICT_NODE *node;
	DICT   *dict;

	if (dict_table == 0)
		acl_msg_fatal("%s(%d): call dict_init first", myname, __LINE__);
	if ((node = dict_node(dict_name)) == 0) {
		if (dict_unknown_allowed == 0)
			acl_msg_fatal("%s: unknown dictionary: %s",
				myname, dict_name);
		dict = dict_ht_open(dict_name, acl_htable_create(0, 0),
				(void (*)(void*)) acl_myfree_fn);
		dict_register(dict_name, dict);
	} else
		dict = node->dict;
	acl_debug(DEBUG_DICT, 2)("%s: sequence func %d", myname, func);
	return dict->sequence(dict, func, key, key_size, value, value_size);
}

/* dict_load_file - read entries from text file */

void    dict_load_file(const char *dict_name, const char *path)
{
	ACL_VSTREAM *fp;
	struct  acl_stat st;
	time_t  before;
	time_t  after;

	/*
	 * Read the file again if it is hot. This may result in reading a
	 * partial parameter name when a file changes in the middle of a read.
	 */
	for (before = time((time_t *) 0); /* see below */ ; before = after) {
		if ((fp = acl_vstream_fopen(path, O_RDONLY, 0600, 0)) == 0)
			acl_msg_fatal("open %s: %s", path, acl_last_serror());
		dict_load_fp(dict_name, fp);
		if (acl_stat(ACL_VSTREAM_PATH(fp), &st) < 0)
			acl_msg_fatal("fstat %s: %s", path, acl_last_serror());
		after = time((time_t *) 0);
		if (st.st_mtime < before - 1 || st.st_mtime > after)
			break;
		acl_debug(DEBUG_DICT, 2)("pausing to let %s cool down", path);
		acl_doze(300000);
	}
}

/* dict_load_fp - read entries from open stream */

void    dict_load_fp(const char *dict_name, ACL_VSTREAM *fp)
{
	ACL_VSTRING *buf;
	char   *member;
	char   *val;
	int     lineno;
	const char *err;

	buf = acl_vstring_alloc(100);
	lineno = 0;

	while (acl_readlline(buf, fp, &lineno)) {
		if ((err = acl_split_nameval(STR(buf), &member, &val)) != 0)
			acl_msg_fatal("%s, line %d: %s: \"%s\"",
				ACL_VSTREAM_PATH(fp), lineno, err, STR(buf));
		dict_update(dict_name, member, val, strlen(val));
	}
	acl_vstring_free(buf);
}

/* dict_eval_lookup - macro parser call-back routine */

static const char *dict_eval_lookup(char *key, int type acl_unused,
	const char *dict_name, char **value, size_t *size)
{
	/*
	 * XXX how would one recover?
	 */
	if (dict_lookup(dict_name, key, value, size) == 0 && dict_errno != 0)
		acl_msg_fatal("dictionary %s: lookup %s: temporary error",
			dict_name, key);

	return *value;
}

static void free_vstring_fn(void *arg)
{
	ACL_VSTRING *buf = (ACL_VSTRING*) arg;

	acl_vstring_free(buf);
}

/* dict_eval - expand embedded dictionary references */

const char *dict_eval(char *dict_name, const char *value, int recursive)
{
	const char *myname = "dict_eval";
	static __thread ACL_VSTRING *buf = NULL;
	int     status;

	/*
	 * Initialize.
	 */
	if (buf == 0) {
		buf = acl_vstring_alloc(10);
		acl_pthread_atexit_add(buf, free_vstring_fn);
	}

	/*
	 * Expand macros, possibly recursively.
	 */
#define DONT_FILTER (char *) 0

	status = mac_expand(buf, value,
			recursive ? MAC_EXP_FLAG_RECURSE : MAC_EXP_FLAG_NONE,
			DONT_FILTER, dict_eval_lookup, (char *) dict_name);
	if (status & MAC_PARSE_ERROR)
		acl_msg_fatal("dictionary %s: macro processing error", dict_name);
	if (acl_do_debug(DEBUG_DICT, 2)) {
		if (strcmp(value, STR(buf)) != 0)
			acl_msg_info("%s: expand %s -> %s", myname, value, STR(buf));
		else
			acl_msg_info("%s: const  %s", myname, value);
	}
	return STR(buf);
}

/* dict_walk - iterate over all dictionaries in arbitrary order */

void    dict_walk(DICT_WALK_ACTION action, char *ptr)
{
	const char *myname = "dict_walk";
	ACL_HTABLE_INFO **ht_info_list;
	ACL_HTABLE_INFO **ht;
	ACL_HTABLE_INFO *h;

	if (dict_table == 0)
		acl_msg_fatal("%s(%d): call dict_init first", myname, __LINE__);
	ht_info_list = acl_htable_list(dict_table);
	for (ht = ht_info_list; (h = *ht) != 0; ht++)
		action(h->key.key, (DICT *) h->value, ptr);
	acl_myfree(ht_info_list);
}

/* dict_changed_name - see if any dictionary has changed */

const char *dict_changed_name(void)
{
	const char *myname = "dict_changed_name";
	struct acl_stat st;
	ACL_HTABLE_INFO **ht_info_list;
	ACL_HTABLE_INFO **ht;
	ACL_HTABLE_INFO *h;
	const char *status;
	char  ebuf[256];
	DICT   *dict;

	if (dict_table == 0)
		acl_msg_fatal("%s(%d): call dict_init first", myname, __LINE__);
	ht_info_list = acl_htable_list(dict_table);
	for (status = 0, ht = ht_info_list; status == 0 && (h = *ht) != 0; ht++) {
		dict = ((DICT_NODE *) h->value)->dict;
		if (dict->stat_fd < 0)			/* not file-based */
			continue;
		if (dict->mtime == 0)			/* not bloody likely */
			acl_msg_warn("%s: table %s: null time stamp",
				myname, h->key.c_key);
		if (acl_stat(dict->db_path, &st) < 0)
			acl_msg_fatal("%s: fstat: %s",
				myname, acl_last_strerror(ebuf, sizeof(ebuf)));
		if (st.st_mtime != dict->mtime || st.st_nlink == 0)
			status = h->key.c_key;
	}
	acl_myfree(ht_info_list);
	return status;
}

/* dict_changed - backwards compatibility */

int     dict_changed(void)
{
	return (dict_changed_name() != 0);
}

/*
 * Mapping between flag names and flag values.
 */
static const NAME_MASK dict_mask[] = {
	{ "warn_dup", (1 << 0), },          /* if file, warn about dups */
	{ "ignore_dup", (1 << 1), },        /* if file, ignore dups */
	{ "try0null", (1 << 2), },          /* do not append 0 to key/value */
	{ "try1null", (1 << 3), },          /* append 0 to key/value */
	{ "fixed", (1 << 4), },             /* fixed key map */
	{ "pattern", (1 << 5), },           /* keys are patterns */
	{ "lock", (1 << 6), },              /* lock before access */
	{ "replace", (1 << 7), },           /* if file, replace dups */
	{ "sync_update", (1 << 8), },       /* if file, sync updates */
	{ "debug", (1 << 9), },             /* log access */
	{ "no_regsub", (1 << 11), },        /* disallow regexp substitution */
	{ "no_proxy", (1 << 12), },         /* disallow proxy mapping */
	{ "no_unauth", (1 << 13), },        /* disallow unauthenticated data */
	{ "fold_fix", (1 << 14), },         /* case-fold with fixed-case key map */
	{ "fold_mul", (1 << 15), },         /* case-fold with multi-case key map */
};

/* dict_flags_str - convert mask to string for debugging purposes */

const char *dict_flags_str(int dict_flags)
{
	static __thread ACL_VSTRING *buf = 0;

	if (buf == 0) {
		buf = acl_vstring_alloc(1);
		acl_pthread_atexit_add(buf, free_vstring_fn);
	}

	return (str_name_mask_opt(buf, "dictionary flags", dict_mask,
			dict_flags, NAME_MASK_RETURN | NAME_MASK_PIPE));
}
