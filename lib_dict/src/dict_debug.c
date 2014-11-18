#include "StdAfx.h"
#include "dict.h"

/* Application-specific. */

typedef struct {
	DICT    dict;			/* the proxy service */
	DICT   *real_dict;		/* encapsulated object */
} DICT_DEBUG;

/* dict_debug_lookup - log lookup operation */

static const char *dict_debug_lookup(DICT *dict, char *key, size_t key_len,
			char **value, size_t *size)
{
	DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;
	const char *result;

	result = DICT_GET(dict_debug->real_dict, key, key_len, value, size);
	acl_msg_info("%s:%s lookup: \"%s\" = \"%s\"", dict->type, dict->name, key,
			result ? result : dict_errno ? "try again" : "not_found");
	return (result);
}

/* dict_debug_update - log update operation */

static void dict_debug_update(DICT *dict, char *key, size_t key_len, char *value, size_t len)
{
	DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;

	acl_msg_info("%s:%s update: \"%s\" = \"%s\"", dict->type, dict->name,
			key, value);
	DICT_PUT(dict_debug->real_dict, key, key_len, value, len);
}

/* dict_debug_delete - log delete operation */

static int dict_debug_delete(DICT *dict, char *key, size_t key_len)
{
	DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;
	int     result;

	result = DICT_DEL(dict_debug->real_dict, key, key_len);
	acl_msg_info("%s:%s delete: \"%s\" = \"%s\"", dict->type, dict->name, key,
			result ? "failed" : "success");
	return (result);
}

/* dict_debug_sequence - log sequence operation */

static int dict_debug_sequence(DICT *dict, int function,
		char **key, size_t *key_size, char **value, size_t *value_size)
{
	DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;
	int     result;

	result = DICT_SEQ(dict_debug->real_dict, function, key, key_size, value, value_size);
	if (result == 0)
		acl_msg_info("%s:%s sequence: \"%s\" = \"%s\"",
				dict->type, dict->name, *key, *value);
	else
		acl_msg_info("%s:%s sequence: found EOF", dict->type, dict->name);
	return (result);
}

/* dict_debug_close - log operation */

static void dict_debug_close(DICT *dict)
{
	DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;

	DICT_CLOSE(dict_debug->real_dict);
	dict_free(dict);
}

/* dict_debug_main - encapsulate dictionary object and install proxies */

DICT   *dict_debug_main(DICT *real_dict)
{
	DICT_DEBUG *dict_debug;

	dict_debug = (DICT_DEBUG *) dict_alloc(real_dict->type,
			real_dict->name, sizeof(*dict_debug));
	dict_debug->dict.flags = real_dict->flags;	/* XXX not synchronized */
	dict_debug->dict.lookup = dict_debug_lookup;
	dict_debug->dict.update = dict_debug_update;
	dict_debug->dict.delete_it = dict_debug_delete;
	dict_debug->dict.sequence = dict_debug_sequence;
	dict_debug->dict.close = dict_debug_close;
	dict_debug->real_dict = real_dict;
	return (&dict_debug->dict);
}
