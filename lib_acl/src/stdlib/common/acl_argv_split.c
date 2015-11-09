#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Application-specific. */

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_mystring.h"

#endif

/* acl_argv_split - split string into token array */

ACL_ARGV   *acl_argv_split(const char *str, const char *delim)
{
	return acl_argv_split3(str, delim, NULL);
}

ACL_ARGV   *acl_argv_split3(const char *str, const char *delim,
	ACL_DBUF_POOL *dbuf)
{
	ACL_ARGV   *argvp = acl_argv_alloc2(1, dbuf);
	char   *saved_string = dbuf ?
		acl_dbuf_pool_strdup(dbuf, str) :
		acl_mystrdup(str);
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

ACL_ARGV   *acl_argv_splitn(const char *str, const char *delim, size_t n)
{
	return acl_argv_splitn4(str, delim, n, NULL);
}

ACL_ARGV   *acl_argv_splitn4(const char *str, const char *delim,
	size_t n, ACL_DBUF_POOL *dbuf)
{
	ACL_ARGV   *argvp = acl_argv_alloc2(n > 0 ? (int) n : 1, dbuf);
	char   *saved_string = dbuf ?
		acl_dbuf_pool_strdup(dbuf, str) :
		acl_mystrdup(str);
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

ACL_ARGV   *acl_argv_split_append(ACL_ARGV *argvp, const char *str,
	const char *delim)
{
	char   *saved_string = argvp->dbuf ?
		acl_dbuf_pool_strdup(argvp->dbuf, str) :
		acl_mystrdup(str);
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

ACL_ARGV   *acl_argv_splitn_append(ACL_ARGV *argvp, const char *str,
	const char *delim, size_t n)
{
	char   *saved_string = argvp->dbuf ?
		acl_dbuf_pool_strdup(argvp->dbuf, str) :
		acl_mystrdup(str);
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
