#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Application-specific. */

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_mystring.h"

#endif

/* acl_argv_split - split string into token array */

ACL_ARGV *acl_argv_split(const char *str, const char *delim)
{
	return acl_argv_split3(str, delim, NULL);
}

ACL_ARGV *acl_argv_split3(const char *str, const char *delim,
	ACL_DBUF_POOL *dbuf)
{
	ACL_ARGV *argvp = acl_argv_alloc2(1, dbuf);
	char   *saved_string =
		dbuf ? acl_dbuf_pool_strdup(dbuf, str) : acl_mystrdup(str);
	char   *bp = saved_string;
	char   *arg;

	while ((arg = acl_mystrtok(&bp, delim)) != 0)
		acl_argv_add(argvp, arg, (char *) 0);
	acl_argv_terminate(argvp);
	if (dbuf)
		acl_dbuf_pool_free(dbuf, saved_string);
	else
		acl_myfree(saved_string);
	return argvp;
}

/* acl_argv_splitn - split string into token array with max items */

ACL_ARGV *acl_argv_splitn(const char *str, const char *delim, size_t n)
{
	return acl_argv_splitn4(str, delim, n, NULL);
}

ACL_ARGV *acl_argv_splitn4(const char *str, const char *delim,
	size_t n, ACL_DBUF_POOL *dbuf)
{
	ACL_ARGV *argvp = acl_argv_alloc2(n > 0 ? (int) n : 1, dbuf);
	char   *saved_string = dbuf ?
		acl_dbuf_pool_strdup(dbuf, str) : acl_mystrdup(str);
	char   *bp = saved_string;
	char   *arg;

	while (n-- > 0 && (arg = acl_mystrtok(&bp, delim)) != 0)
		acl_argv_add(argvp, arg, (char *) 0);
	acl_argv_terminate(argvp);
	if (dbuf)
		acl_dbuf_pool_free(dbuf, saved_string);
	else
		acl_myfree(saved_string);
	return argvp;
}

/* acl_argv_split_append - split string into token array, append to array */

ACL_ARGV *acl_argv_split_append(ACL_ARGV *argvp, const char *str,
	const char *delim)
{
	char   *saved_string = argvp->dbuf ?
		acl_dbuf_pool_strdup(argvp->dbuf, str) : acl_mystrdup(str);
	char   *bp = saved_string;
	char   *arg;

	while ((arg = acl_mystrtok(&bp, delim)) != 0)
		acl_argv_add(argvp, arg, (char *) 0);
	acl_argv_terminate(argvp);
	if (argvp->dbuf)
		acl_dbuf_pool_free(argvp->dbuf, saved_string);
	else
		acl_myfree(saved_string);
	return argvp;
}

/* acl_argv_splitn_append - split string into token array, append to
 * array with max items */

ACL_ARGV *acl_argv_splitn_append(ACL_ARGV *argvp, const char *str,
	const char *delim, size_t n)
{
	char   *saved_string = argvp->dbuf ?
		acl_dbuf_pool_strdup(argvp->dbuf, str) : acl_mystrdup(str);
	char   *bp = saved_string;
	char   *arg;

	while (n-- > 0 && (arg = acl_mystrtok(&bp, delim)) != 0)
		acl_argv_add(argvp, arg, (char *) 0);
	acl_argv_terminate(argvp);
	if (argvp->dbuf)
		acl_dbuf_pool_free(argvp->dbuf, saved_string);
	else
		acl_myfree(saved_string);
	return argvp;
}

ACL_ARGV *acl_argv_quote_split(const char *str, const char *delim)
{
	return acl_argv_quote_split4(str, delim, NULL);
}

ACL_ARGV *acl_argv_quote_split4(const char *str, const char *delim,
	ACL_DBUF_POOL *dbuf)
{
	ACL_ARGV *argvp = acl_argv_alloc2(1, dbuf);
	int     ch, quote = 0, backslash = 0;
	ACL_VSTRING *buf = acl_vstring_dbuf_alloc(dbuf, 128);

#define	ADDCH	ACL_VSTRING_ADDCH
#define	TERM	ACL_VSTRING_TERMINATE
#define	LEN	ACL_VSTRING_LEN
#define	STR	acl_vstring_str
#define	RESET	ACL_VSTRING_RESET

	while ((ch = *str) != 0) {
		if (quote) {
			if (backslash) {
				ADDCH(buf, ch);
				TERM(buf);
				backslash = 0;
			} else if (ch == '\\')
				backslash = 1;
			else if (ch == quote) {
				quote = 0;
				if (LEN(buf) > 0) {
					acl_argv_add(argvp, STR(buf), NULL);
					RESET(buf);
				}
			} else {
				ADDCH(buf, ch);
				TERM(buf);
			}
		} else if (backslash) {
			ADDCH(buf, ch);
			TERM(buf);
			backslash = 0;
		} else if (ch == '\\')
			backslash = 1;
		else if (ch == '\'' || ch == '\"')
			quote = ch;
		else if (strchr(delim, ch) != NULL) {
			if (LEN(buf) > 0) {
				acl_argv_add(argvp, STR(buf), NULL);
				RESET(buf);
			}
		} else {
			ADDCH(buf, ch);
			TERM(buf);
		}

		str++;;
	}

	if (LEN(buf) > 0)
		acl_argv_add(argvp, STR(buf), NULL);

	acl_argv_terminate(argvp);

	acl_vstring_free(buf);

	return argvp;
}
