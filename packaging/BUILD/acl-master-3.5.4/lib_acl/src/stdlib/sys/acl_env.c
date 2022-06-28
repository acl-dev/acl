#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef	ACL_UNIX
#include <unistd.h>
#endif
#include <string.h>

/* Utility library. */

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_safe.h"
#include "thread/acl_pthread.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_env.h"

#endif

/* acl_clean_env - clean up the environment */

void acl_clean_env(char **preserve_list)
{
#ifdef ACL_UNIX
	extern char **environ;
	char  **env = environ;
#endif
	ACL_ARGV   *save_list;
	char   *value;
	char  **cpp;
	char   *eq;

	/*
	 * Preserve or specify selected environment variables.
	 */
#define STRING_AND_LENGTH(x, y) (x), (ssize_t) (y)

	save_list = acl_argv_alloc(10);
	for (cpp = preserve_list; *cpp; cpp++) {
		if ((eq = strchr(*cpp, '=')) != 0)
			acl_argv_addn(save_list,
				STRING_AND_LENGTH(*cpp, eq - *cpp),
				STRING_AND_LENGTH(eq + 1, strlen(eq + 1)),
				(char *) 0);
		else if ((value = acl_safe_getenv(*cpp)) != 0)
			acl_argv_add(save_list, *cpp, value, (char *) 0);
	}

	/*
	 * Truncate the process environment, if available. On some systems
	 * (Ultrix!), environ can be a null pointer.
	 */
#ifdef ACL_UNIX
	if (env)
		env[0] = 0;
#endif

	/*
	 * Restore preserved environment variables.
	 */
	for (cpp = save_list->argv; *cpp; cpp += 2)
#ifdef ACL_UNIX
		if (setenv(cpp[0], cpp[1], 1))
#elif defined(ACL_WINDOWS)
		if (!SetEnvironmentVariable(cpp[0], cpp[1]))
#endif
			acl_msg_error("setenv(%s, %s): %s",
				cpp[0], cpp[1], acl_last_serror());

	/*
	 * Cleanup.
	 */
	acl_argv_free(save_list);
}

char *acl_getenv(const char *name)
{
#ifdef ACL_WINDOWS
	const char *myname = "acl_getenv";
	static acl_pthread_key_t buf_key = ACL_TLS_OUT_OF_INDEXES;
	char *buf;
#define	ENV_BUF_SIZE	4096

	buf = (char*) acl_pthread_tls_get(&buf_key);
	if (buf == NULL) {
		if (buf_key == ACL_TLS_OUT_OF_INDEXES) {
			acl_msg_error("%s(%d): acl_pthread_tls_get error(%s)",
				myname, __LINE__, acl_last_serror());
			return (NULL);
		}
		buf = (char*) acl_mymalloc(ENV_BUF_SIZE);
		acl_pthread_tls_set(buf_key, buf, (void (*)(void*)) acl_myfree_fn);
	}

	if (GetEnvironmentVariable(name, buf, ENV_BUF_SIZE) == 0)
		return (NULL);
	return (buf);
#else
	return (getenv(name));
#endif
}

char *acl_getenv3(const char *name, char *buf, size_t len)
{
#ifdef ACL_WINDOWS
	if (GetEnvironmentVariable(name, buf, (DWORD) len) == 0)
		return (NULL);
	return (buf);
#else
	const char *ptr = getenv(name);
	if (ptr == NULL)
		return (NULL);
	ACL_SAFE_STRNCPY(buf, ptr, len);
	return (buf);
#endif
}

int acl_setenv(const char *name, const char *val, int overwrite)
{
#ifdef ACL_WINDOWS
	if (overwrite == 0) {
		if (acl_getenv(name) != NULL)
			return (0);
	}
	if (!SetEnvironmentVariable(name, val))
		return (-1);
	return (0);
#else
	return (setenv(name, val, overwrite));
#endif
}

int acl_putenv(char *str)
{
#ifdef ACL_WINDOWS
	const char *myname = "acl_putenv";
	ACL_ARGV *argv = acl_argv_split(str, "=");

	if (argv->argc != 2) {
		acl_msg_error("%s(%d): input(%s) invalid", myname, __LINE__, str);
		return (-1);
	}
	if (!SetEnvironmentVariable(argv->argv[0], argv->argv[1])) {
		acl_msg_error("%s(%d): putenv(%s, %s) error(%s)",
			myname, __LINE__, argv->argv[0],
			argv->argv[1], acl_last_serror());
		return (-1);
	}
	return (0);
#else
	return (putenv(str));
#endif
}

static void free_vstring(ACL_VSTRING *s)
{
	acl_vstring_free(s);
}

const char *acl_getenv_list(void)
{
	const char *myname = "acl_getenv_list";
#ifdef ACL_WINDOWS
	static acl_pthread_key_t buf_key = ACL_TLS_OUT_OF_INDEXES;
	ACL_VSTRING *buf;
	LPTSTR lpszVariable;
	LPVOID lpvEnv;
	int   i = 0, ch = 0;

	buf = (ACL_VSTRING*) acl_pthread_tls_get(&buf_key);
	if (buf == NULL) {
		if (buf_key == ACL_TLS_OUT_OF_INDEXES) {
			acl_msg_error("%s(%d): acl_pthread_tls_get error(%s)",
				myname, __LINE__, acl_last_serror());
			return (NULL);
		}
		buf = acl_vstring_alloc(256);
		acl_pthread_tls_set(buf_key, buf, (void (*)(void*)) free_vstring);
	} else
		ACL_VSTRING_RESET(buf);

	lpvEnv = GetEnvironmentStrings();
	for (lpszVariable = (LPTSTR) lpvEnv; *lpszVariable; lpszVariable++) {
		if (i++ > 0)
			acl_vstring_strcat(buf, ", ");
		while (*lpszVariable) {
			ACL_VSTRING_ADDCH(buf, *lpszVariable++);
			ch = *lpszVariable;
		}
	}
	FreeEnvironmentStrings(lpvEnv);
	ACL_VSTRING_TERMINATE(buf);
	return (acl_vstring_str(buf));
#else
	static acl_pthread_key_t buf_key =
		(acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
	ACL_VSTRING *buf;
	extern char **environ;
	char **pptr = environ;
	int   i = 0;

	buf = (ACL_VSTRING*) acl_pthread_tls_get(&buf_key);
	if (buf == NULL) {
		if (buf_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
			acl_msg_error("%s(%d): acl_pthread_tls_get error(%s)",
				myname, __LINE__, acl_last_serror());
			return (NULL);
		}
		buf = acl_vstring_alloc(256);
		acl_pthread_tls_set(buf_key, buf, (void (*)(void*)) free_vstring);
	} else
		ACL_VSTRING_RESET(buf);

	while (*pptr) {
		if (i++ > 0)
			acl_vstring_strcat(buf, ", ");
		acl_vstring_strcat(buf, *pptr);
		pptr++;
	}

	return (acl_vstring_str(buf));
#endif
}
