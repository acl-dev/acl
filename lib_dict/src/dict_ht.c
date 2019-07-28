#include "StdAfx.h"
#include "dict.h"
#include "dict_ht.h"

/* Application-specific. */

typedef struct {
	DICT    dict;			/* generic members */
	ACL_HTABLE *table;		/* hash table */
	void    (*remove_fn) (void *);	/* callback */
} DICT_HT;

/* dict_ht_lookup - find hash-table entry */

static const char *dict_ht_lookup(DICT *dict, char *name, size_t name_len acl_unused,
	char **value, size_t *size)
{
	DICT_HT *dict_ht = (DICT_HT *) dict;

	dict_errno = 0;

	*value = acl_htable_find(dict_ht->table, name);
	if (*value)
		*size = strlen(*value);
	return (*value);
}

/* dict_ht_update - add or update hash-table entry */

static void dict_ht_update(DICT *dict, char *name, size_t name_len acl_unused, char *value, size_t len)
{
	DICT_HT *dict_ht = (DICT_HT *) dict;
	ACL_HTABLE_INFO *ht;

	if ((ht = acl_htable_locate(dict_ht->table, name)) != 0) {
		acl_myfree(ht->value);
	} else {
		ht = acl_htable_enter(dict_ht->table, name, (char *) 0);
	}
	ht->value = acl_mymemdup(value, len);
}

/* dict_ht_close - disassociate from hash table */

static void dict_ht_close(DICT *dict)
{
	DICT_HT *dict_ht = (DICT_HT *) dict;

	if (dict_ht->remove_fn)
		acl_htable_free(dict_ht->table, dict_ht->remove_fn);
	dict_free(dict);
}

/* dict_ht_open - create association with hash table */

DICT   *dict_ht_open(const char *name, ACL_HTABLE *table, void (*remove_fn) (void *))
{
	DICT_HT *dict_ht;

	dict_ht = (DICT_HT *) dict_alloc(DICT_TYPE_HT, name, sizeof(*dict_ht));
	dict_ht->dict.lookup = dict_ht_lookup;
	dict_ht->dict.update = dict_ht_update;
	dict_ht->dict.close = dict_ht_close;
	dict_ht->table = table;
	dict_ht->remove_fn = remove_fn;
	return (&dict_ht->dict);
}
