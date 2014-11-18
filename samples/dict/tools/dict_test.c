/*
 * Proof-of-concept test program. Create, update or read a database. When
 * the input is a name=value pair, the database is updated, otherwise the
 * program assumes that the input specifies a lookup key and prints the
 * corresponding value.
 */

/* System library. */
#include "lib_acl.h"
#include <stdlib.h>
#include <fcntl.h>
#ifdef ACL_UNIX
#include <unistd.h>
#include <signal.h>
#endif
#include "dict.h"
#include "unescape.h"
#include "dict_test.h"

#ifdef ACL_MS_WINDOWS
#include <io.h>
#define isatty _isatty
#endif

static void dict_test1(const char *dict_name, int open_flags, int dict_flags);
static void dict_test2(const char *dict_name, int open_flags, int dict_flags);

static void usage(char *myname)
{
	acl_msg_fatal("usage: %s -o test type:file read|write|create [fold]", myname);
}

static void help(void)
{
	acl_vstream_printf("usage: del key|get key|put key=value|first|next|exit\r\n");
}

int dict_test_main(int argc, char **argv)
{
	char   *dict_name;
	int     open_flags = 0;
	int     dict_flags = /* DICT_FLAG_LOCK | */ DICT_FLAG_DUP_REPLACE;
	int     ch;

	/*
	 * signal(SIGPIPE, SIG_IGN);
	 */

	acl_init();

	while ((ch = getopt(argc, argv, "v")) > 0) {
		switch (ch) {
			default:
				usage(argv[0]);
			case 'v':
				break;
		}
	}

	if (argc - optind < 2 || argc - optind > 3)
		usage(argv[0]);
	if (strcasecmp(argv[optind + 1], "create") == 0)
		open_flags = O_CREAT | O_RDWR; /* | O_TRUNC */
	else if (strcasecmp(argv[optind + 1], "write") == 0)
		open_flags = O_RDWR;
	else if (strcasecmp(argv[optind + 1], "read") == 0)
		open_flags = O_RDONLY;
	else
		acl_msg_fatal("unknown access mode: %s", argv[2]);
	if (argv[optind + 2] && strcasecmp(argv[optind + 2], "fold") == 0)
		dict_flags |= DICT_FLAG_FOLD_ANY;
	dict_name = argv[optind];

	if (0)
		dict_test1(dict_name, open_flags, dict_flags);
	else
		dict_test2(dict_name, open_flags, dict_flags);
	return (0);
}

static void dict_test1(const char *dict_name, int open_flags, int dict_flags)
{
	char   *bufp;
	char   *cmd;
	char   *key;
	char   *value;
	ACL_VSTRING *keybuf = acl_vstring_alloc(1);
	ACL_VSTRING *inbuf = acl_vstring_alloc(1);
	DICT   *dict;
	size_t  key_size, value_size;

	dict_open_init();
	dict = dict_open(dict_name, open_flags, dict_flags);

	while (1) {
		acl_vstream_printf("Please input: ");
		if (acl_vstring_fgets_nonl(inbuf, ACL_VSTREAM_IN) == NULL)
			break;
		bufp = acl_vstring_str(inbuf);
		if (!isatty(0)) {
			acl_vstream_printf("> %s\n", bufp);
		}
		if (*bufp == '#')
			continue;
		if ((cmd = acl_mystrtok(&bufp, " ")) == 0) {
			help();
			continue;
		}
		if (dict_changed_name())
			acl_msg_warn("dictionary has changed");
		key = *bufp ? acl_vstring_str(unescape(keybuf, acl_mystrtok(&bufp, " ="))) : 0;
		value = acl_mystrtok(&bufp, " =");
		if (strcmp(cmd, "exit") == 0)
			break;
		else if (strcmp(cmd, "del") == 0 && key && !value) {
			if (DICT_DEL(dict, key, strlen(key)))
				acl_vstream_printf("%s: not found\n", key);
			else
				acl_vstream_printf("%s: deleted\n", key);
		} else if (strcmp(cmd, "get") == 0 && key && !value) {
			if (DICT_GET(dict, key, strlen(key), &value, &value_size) == NULL) {
				acl_vstream_printf("%s: %s\n", key,
					dict_errno == DICT_ERR_RETRY ? "soft error" : "not found");
			} else {
				acl_vstream_printf("%s=%s\n", key, value);
				acl_myfree(value);
			}
		} else if (strcmp(cmd, "put") == 0 && key && value) {
			DICT_PUT(dict, key, strlen(key), value, strlen(value));
			acl_vstream_printf("%s=%s\n", key, value);
		} else if (strcmp(cmd, "first") == 0 && !key && !value) {
			if (DICT_SEQ(dict, DICT_SEQ_FUN_FIRST, &key, &key_size,
				&value, &value_size) == 0) {
				acl_vstream_printf("%s=%s\n", key, value);
				acl_myfree(key);
				acl_myfree(value);
			} else
				acl_vstream_printf("%s\n",
					dict_errno == DICT_ERR_RETRY ? "soft error" : "not found");
		} else if (strcmp(cmd, "next") == 0 && !key && !value) {
			if (DICT_SEQ(dict, DICT_SEQ_FUN_NEXT, &key, &key_size, &value, &value_size) == 0) {
				acl_vstream_printf("%s=%s\n", key, value);
				acl_myfree(key);
				acl_myfree(value);
			} else {
				acl_vstream_printf("%s\n",
					dict_errno == DICT_ERR_RETRY ? "soft error" : "not found");
			}
		} else {
			help();
		}
	}
	acl_vstring_free(keybuf);
	acl_vstring_free(inbuf);
	DICT_CLOSE(dict);
}

static void dict_test2(const char *dict_name, int open_flags, int dict_flags)
{
	char   *bufp;
	char   *cmd;
	char   *key;
	char   *value;
	ACL_VSTRING *keybuf = acl_vstring_alloc(1);
	ACL_VSTRING *inbuf = acl_vstring_alloc(1);
	DICT   *dict;
	size_t  key_size, value_size;

	dict_init();

	dict = dict_open(dict_name, open_flags, dict_flags);
	dict_register(dict_name, dict);

	while (1) {
		acl_vstream_printf("Please input: ");
		if (acl_vstring_fgets_nonl(inbuf, ACL_VSTREAM_IN) == NULL)
			break;
		bufp = acl_vstring_str(inbuf);
		if (!isatty(0)) {
			acl_vstream_printf("> %s\n", bufp);
		}
		if (*bufp == '#')
			continue;
		if ((cmd = acl_mystrtok(&bufp, " ")) == 0) {
			help();
			continue;
		}
		if (dict_changed_name())
			acl_msg_warn("dictionary has changed");
		key = *bufp ? acl_vstring_str(unescape(keybuf, acl_mystrtok(&bufp, " ="))) : 0;
		value = acl_mystrtok(&bufp, " =");
		if (strcmp(cmd, "exit") == 0)
			break;
		else if (strcmp(cmd, "del") == 0 && key && !value) {
			if (dict_delete(dict_name, key))
				acl_vstream_printf("%s: not found\n", key);
			else
				acl_vstream_printf("%s: deleted\n", key);
		} else if (strcmp(cmd, "get") == 0 && key && !value) {
			if (dict_lookup(dict_name, key, &value, &value_size) == NULL) {
				acl_vstream_printf("%s: %s\n", key,
					dict_errno == DICT_ERR_RETRY ? "soft error" : "not found");
			} else {
				acl_vstream_printf("%s=%s\n", key, value);
				acl_myfree(value);
			}
		} else if (strcmp(cmd, "put") == 0 && key && value) {
			dict_update(dict_name, key, value, strlen(value));
			acl_vstream_printf("%s=%s\n", key, value);
		} else if (strcmp(cmd, "first") == 0 && !key && !value) {
			if (dict_sequence(dict_name, DICT_SEQ_FUN_FIRST, &key, &key_size,
				&value, &value_size) == 0) {
				acl_vstream_printf("%s=%s\n", key, value);
				acl_myfree(key);
				acl_myfree(value);
			} else
				acl_vstream_printf("%s\n",
					dict_errno == DICT_ERR_RETRY ? "soft error" : "not found");
		} else if (strcmp(cmd, "next") == 0 && !key && !value) {
			if (dict_sequence(dict_name, DICT_SEQ_FUN_NEXT, &key, &key_size,
				&value, &value_size) == 0) {
				acl_vstream_printf("%s=%s\n", key, value);
				acl_myfree(key);
				acl_myfree(value);
			} else {
				acl_vstream_printf("%s\n",
					dict_errno == DICT_ERR_RETRY ? "soft error" : "not found");
			}
		} else {
			help();
		}
	}
	acl_vstring_free(keybuf);
	acl_vstring_free(inbuf);
	dict_unregister(dict_name);
}
