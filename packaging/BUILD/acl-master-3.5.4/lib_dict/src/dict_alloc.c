#include "StdAfx.h"
#include "dict.h"

/* dict_default_lookup - trap unimplemented operation */

static const char *dict_default_lookup(DICT *dict, char *key acl_unused,
	size_t key_len acl_unused, char **value acl_unused,
	size_t *size acl_unused)
{
	acl_msg_fatal("%s table %s: lookup operation is not supported",
		dict->type, dict->name);
	return NULL;
}

/* dict_default_update - trap unimplemented operation */

static void dict_default_update(DICT *dict, char *key acl_unused,
	size_t key_len acl_unused, char *value acl_unused, size_t len acl_unused)
{
	acl_msg_fatal("%s table %s: update operation is not supported",
		dict->type, dict->name);
}

/* dict_default_delete - trap unimplemented operation */

static int dict_default_delete(DICT *dict, char *key acl_unused,
	size_t key_len acl_unused)
{
	acl_msg_fatal("%s table %s: delete operation is not supported",
		dict->type, dict->name);
	/* not reached */
	return -1;
}

/* dict_default_sequence_reset - trap unimplemented operation */

static void dict_default_sequence_reset(DICT *dict)
{
	acl_msg_fatal("%s table %s: sequence_reset operation is not supported",
		dict->type, dict->name);
}

/* dict_default_sequence_delcur - trap unimplemented operation */

static int dict_default_sequence_delcur(DICT *dict)
{
	acl_msg_fatal("%s table %s: sequence_delcur operation is not supported",
		dict->type, dict->name);
	/* not reached */
	return -1;
}

/* dict_default_sequence - trap unimplemented operation */

static int dict_default_sequence(DICT *dict, int function acl_unused,
	char **key acl_unused, size_t *key_size acl_unused,
	char **value acl_unused, size_t *value_size acl_unused)
{
	acl_msg_fatal("%s table %s: sequence operation is not supported",
		dict->type, dict->name);
	/* not reached */
	return -1;
}

/* dict_default_close - trap unimplemented operation */

static void dict_default_close(DICT *dict)
{
	acl_msg_fatal("%s table %s: close operation is not supported",
		dict->type, dict->name);
}

/* dict_alloc - allocate dictionary object, initialize super-class */

DICT   *dict_alloc(const char *dict_type, const char *dict_name, size_t size)
{
	DICT   *dict = (DICT *) acl_mymalloc(size);

	dict->type            = acl_mystrdup(dict_type);
	dict->name            = acl_mystrdup(dict_name);
	dict->flags           = DICT_FLAG_FIXED;
	dict->lookup          = dict_default_lookup;
	dict->update          = dict_default_update;
	dict->delete_it       = dict_default_delete;
	dict->sequence        = dict_default_sequence;
	dict->sequence_reset  = dict_default_sequence_reset;
	dict->sequence_delcur = dict_default_sequence_delcur;
	dict->close           = dict_default_close;
	dict->lock_fd         = ACL_FILE_INVALID;
	dict->stat_fd         = ACL_FILE_INVALID;
	dict->mtime           = 0;
	dict->fold_buf        = 0;
	return dict;
}

/* dict_free - super-class destructor */

void    dict_free(DICT *dict)
{
	acl_myfree(dict->type);
	acl_myfree(dict->name);
	acl_myfree(dict);
}
