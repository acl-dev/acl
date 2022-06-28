#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Application-specific. */

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_dbuf_pool.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_argv.h"

#endif

#define SPACE_LEFT(a) ((a)->len - (a)->argc - 1)

/* argv_extend - extend array */

static void argv_extend(ACL_ARGV *argvp)
{
	int     new_len;

	new_len = argvp->len * 2;
	if (argvp->dbuf) {
		char** argv = (char **) acl_dbuf_pool_alloc(argvp->dbuf,
				(new_len + 1) * sizeof(char *));
		memcpy(argv, argvp->argv, argvp->len * sizeof(char*));
		acl_dbuf_pool_free(argvp->dbuf, argvp->argv);
		argvp->argv = argv;
	}
	else
		argvp->argv = (char **) acl_myrealloc((char *) argvp->argv,
			(new_len + 1) * sizeof(char *));
	argvp->len = new_len;
}

static void argv_push_back(struct ACL_ARGV *argvp, const char *s)
{
	acl_argv_add(argvp, s, 0);
}

static void argv_push_front(struct ACL_ARGV *argvp, const char *s)
{
	int   i;

	/* Make sure that always argvp->argc < argvp->len. */

	if (SPACE_LEFT(argvp) <= 0)
		argv_extend(argvp);
	for (i = argvp->argc; i > 0; i--) {
		argvp->argv[i] = argvp->argv[i - 1];
	}
	if (argvp->dbuf)
		argvp->argv[0] = acl_dbuf_pool_strdup(argvp->dbuf, s);
	else
		argvp->argv[0] = acl_mystrdup(s);
	argvp->argc++;
}

static char *argv_pop_back(struct ACL_ARGV *argvp)
{
	if (argvp->argc <= 0)
		return (NULL);
	return argvp->argv[--argvp->argc];
}

static char *argv_pop_front(struct ACL_ARGV *argvp)
{
	char *s;
	int   i;

	if (argvp->argc <= 0)
		return (NULL);
	s = argvp->argv[0];
	argvp->argc--;
	for (i = 0; i < argvp->argc; i++)
		argvp->argv[i] = argvp->argv[i + 1];

	return s;
}

/* argv_iter_head - get the head of the array */

static void *argv_iter_head(ACL_ITER *iter, struct ACL_ARGV *argv)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;

	iter->i = 0;
	iter->size = argv->argc;
	iter->ptr = argv->argv[0];

	iter->data = iter->ptr;
	return iter->ptr;
}

/* argv_iter_next - get the next of the array */

static void *argv_iter_next(ACL_ITER *iter, struct ACL_ARGV *argv)
{
	iter->i++;
	if (iter->i >= argv->argc)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = argv->argv[iter->i];
	return iter->ptr;
}
 
/* argv_iter_tail - get the tail of the array */

static void *argv_iter_tail(ACL_ITER *iter, struct ACL_ARGV *argv)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;

	iter->i = argv->argc - 1;
	iter->size = argv->argc;
	if (iter->i < 0)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = argv->argv[iter->i];
	return iter->ptr;
}

/* argv_iter_prev - get the prev of the array */

static void *argv_iter_prev(ACL_ITER *iter, struct ACL_ARGV *argv)
{
	iter->i--;
	if (iter->i < 0)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = argv->argv[iter->i];
	return iter->ptr;
}

/* acl_argv_free - destroy string array */

ACL_ARGV   *acl_argv_free(ACL_ARGV *argvp)
{
	char  **cpp;

	for (cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++) {
		if (argvp->dbuf)
			acl_dbuf_pool_free(argvp->dbuf, *cpp);
		else
			acl_myfree(*cpp);
	}
	if (argvp->dbuf) {
		acl_dbuf_pool_free(argvp->dbuf, argvp->argv);
		acl_dbuf_pool_free(argvp->dbuf, argvp);
	} else {
		acl_myfree(argvp->argv);
		acl_myfree(argvp);
	}
	return NULL;
}

/* acl_argv_alloc - initialize string array */

ACL_ARGV   *acl_argv_alloc(int size)
{
	return acl_argv_alloc2(size, NULL);
}

ACL_ARGV   *acl_argv_alloc2(int len, ACL_DBUF_POOL *dbuf)
{
	ACL_ARGV   *argvp;
	int     sane_len;

	/* Make sure that always argvp->argc < argvp->len. */

	if (dbuf) {
		argvp = (ACL_ARGV *) acl_dbuf_pool_alloc(dbuf, sizeof(*argvp));
		argvp->dbuf = dbuf;
	} else {
		argvp = (ACL_ARGV *) acl_mymalloc(sizeof(*argvp));
		argvp->dbuf = NULL;
	}
	argvp->len = 0;
	sane_len = (len < 2 ? 2 : len);
	if (argvp->dbuf)
		argvp->argv = (char **) acl_dbuf_pool_alloc(argvp->dbuf,
				(sane_len + 1) * sizeof(char *));
	else
		argvp->argv = (char **)
			acl_mymalloc((sane_len + 1) * sizeof(char *));
	argvp->len = sane_len;
	argvp->argc = 0;
	argvp->argv[0] = 0;

	argvp->push_back = argv_push_back;
	argvp->push_front = argv_push_front;
	argvp->pop_back = argv_pop_back;
	argvp->pop_front = argv_pop_front;

	/* set the iterator callback */
	argvp->iter_head = argv_iter_head;
	argvp->iter_next = argv_iter_next;
	argvp->iter_tail = argv_iter_tail;
	argvp->iter_prev = argv_iter_prev;

	return argvp;
}

/* acl_argv_add - add string to vector */

void    acl_argv_add(ACL_ARGV *argvp,...)
{
	const char *arg;
	va_list ap;

	/* Make sure that always argvp->argc < argvp->len. */

	va_start(ap, argvp);
	while ((arg = va_arg(ap, const char *)) != 0) {
		if (SPACE_LEFT(argvp) <= 0)
			argv_extend(argvp);
		if (argvp->dbuf)
			argvp->argv[argvp->argc++] = acl_dbuf_pool_strdup(
				argvp->dbuf, arg);
		else
			argvp->argv[argvp->argc++] = acl_mystrdup(arg);
	}
	va_end(ap);
	argvp->argv[argvp->argc] = 0;
}

void    acl_argv_addv(ACL_ARGV *argvp, va_list ap)
{
	const char *arg;

	/* Make sure that always argvp->argc < argvp->len. */

	while ((arg = va_arg(ap, const char *)) != 0) {
		if (SPACE_LEFT(argvp) <= 0)
			argv_extend(argvp);
		if (argvp->dbuf)
			argvp->argv[argvp->argc++] = acl_dbuf_pool_strdup(
				argvp->dbuf, arg);
		else
			argvp->argv[argvp->argc++] = acl_mystrdup(arg);
	}
	argvp->argv[argvp->argc] = 0;
}

/* acl_argv_addn - add string to vector */

void    acl_argv_addn(ACL_ARGV *argvp,...)
{
	const char *myname = "acl_argv_addn";
	const char *arg;
	int     len;
	va_list ap;

	/* Make sure that always argvp->argc < argvp->len. */

	va_start(ap, argvp);
	while ((arg = va_arg(ap, const char *)) != 0) {
		if ((len = va_arg(ap, int)) < 0)
			acl_msg_panic("%s: bad string length %d", myname, len);
		if (SPACE_LEFT(argvp) <= 0)
			argv_extend(argvp);
		if (argvp->dbuf)
			argvp->argv[argvp->argc++] = acl_dbuf_pool_strndup(
				argvp->dbuf, arg, len);
		else
			argvp->argv[argvp->argc++] = acl_mystrndup(arg, len);
	}
	va_end(ap);
	argvp->argv[argvp->argc] = 0;
}

void    acl_argv_addnv(ACL_ARGV *argvp, va_list ap)
{
	const char *myname = "acl_argv_addnv";
	const char *arg;
	int     len;

	/* Make sure that always argvp->argc < argvp->len. */

	while ((arg = va_arg(ap, const char *)) != 0) {
		if ((len = va_arg(ap, int)) < 0)
			acl_msg_panic("%s: bad string length %d", myname, len);
		if (SPACE_LEFT(argvp) <= 0)
			argv_extend(argvp);
		if (argvp->dbuf)
			argvp->argv[argvp->argc++] = acl_dbuf_pool_strndup(
				argvp->dbuf, arg, len);
		else
			argvp->argv[argvp->argc++] = acl_mystrndup(arg, len);
	}
	argvp->argv[argvp->argc] = 0;
}

int     acl_argv_set(ACL_ARGV *argvp, int idx, const char *value)
{
	if (idx < 0 || idx >= argvp->argc)
		return -1;
	if (value == NULL)
		return -1;

	if (argvp->dbuf)
		argvp->argv[idx] = acl_dbuf_pool_strdup(argvp->dbuf, value);
	else {
		acl_myfree(argvp->argv[idx]);
		argvp->argv[idx] = acl_mystrdup(value);
	}

	return 0;
}

/* acl_argv_terminate - terminate string array */

void    acl_argv_terminate(ACL_ARGV *argvp)
{
	/* Trust that argvp->argc < argvp->len. */
	argvp->argv[argvp->argc] = 0;
}

char  *acl_argv_index(ACL_ARGV *argvp, int idx)
{
	if (argvp == NULL || idx < 0 || idx > argvp->argc - 1)
		return(NULL);

	return argvp->argv[idx];
}

int   acl_argv_size(ACL_ARGV *argvp)
{
	if (argvp == NULL)
		return -1;

	return argvp->argc;
}

