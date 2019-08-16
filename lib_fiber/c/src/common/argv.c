#include "stdafx.h"
#include "memory.h"
#include "strops.h"
#include "argv.h"

#define SPACE_LEFT(a) ((a)->len - (a)->argc - 1)

/* argv_extend - extend array */

static void argv_extend(ARGV *argvp)
{
	int new_len;

	new_len = argvp->len * 2;
	argvp->argv = (char **) mem_realloc((char *) argvp->argv,
			(new_len + 1) * sizeof(char *));
	argvp->len = new_len;
}

static void argv_push_back(struct ARGV *argvp, const char *s)
{
	argv_add(argvp, s, 0);
}

static void argv_push_front(struct ARGV *argvp, const char *s)
{
	int   i;

	/* Make sure that always argvp->argc < argvp->len. */

	if (SPACE_LEFT(argvp) <= 0) {
		argv_extend(argvp);
	}
	for (i = argvp->argc; i > 0; i--) {
		argvp->argv[i] = argvp->argv[i - 1];
	}

	argvp->argv[0] = STRDUP(s);
	argvp->argc++;
}

static char *argv_pop_back(struct ARGV *argvp)
{
	if (argvp->argc <= 0) {
		return NULL;
	}
	return argvp->argv[--argvp->argc];
}

static char *argv_pop_front(struct ARGV *argvp)
{
	char *s;
	int   i;

	if (argvp->argc <= 0) {
		return (NULL);
	}
	s = argvp->argv[0];
	argvp->argc--;
	for (i = 0; i < argvp->argc; i++) {
		argvp->argv[i] = argvp->argv[i + 1];
	}
	return s;
}

/* argv_iter_head - get the head of the array */

static void *argv_iter_head(ITER *iter, struct ARGV *argv)
{
	iter->dlen = -1;
	iter->key  = NULL;
	iter->klen = -1;

	iter->i    = 0;
	iter->size = argv->argc;
	iter->ptr  = argv->argv[0];

	iter->data = iter->ptr;
	return iter->ptr;
}

/* argv_iter_next - get the next of the array */

static void *argv_iter_next(ITER *iter, struct ARGV *argv)
{
	iter->i++;
	if (iter->i >= argv->argc) {
		iter->data = iter->ptr = 0;
	} else {
		iter->data = iter->ptr = argv->argv[iter->i];
	}
	return iter->ptr;
}
 
/* argv_iter_tail - get the tail of the array */

static void *argv_iter_tail(ITER *iter, struct ARGV *argv)
{
	iter->dlen = -1;
	iter->key  = NULL;
	iter->klen = -1;

	iter->i    = argv->argc - 1;
	iter->size = argv->argc;
	if (iter->i < 0) {
		iter->data = iter->ptr = 0;
	} else {
		iter->data = iter->ptr = argv->argv[iter->i];
	}
	return iter->ptr;
}

/* argv_iter_prev - get the prev of the array */

static void *argv_iter_prev(ITER *iter, struct ARGV *argv)
{
	iter->i--;
	if (iter->i < 0) {
		iter->data = iter->ptr = 0;
	} else {
		iter->data = iter->ptr = argv->argv[iter->i];
	}
	return iter->ptr;
}

/* argv_free - destroy string array */

ARGV   *argv_free(ARGV *argvp)
{
	char  **cpp;

	for (cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++) {
		mem_free(*cpp);
	}
	mem_free(argvp->argv);
	mem_free(argvp);
	return NULL;
}

/* argv_alloc - initialize string array */

ARGV   *argv_alloc(int size)
{
	ARGV   *argvp;
	int     sane_len;

	/* Make sure that always argvp->argc < argvp->len. */

	argvp          = (ARGV *) mem_malloc(sizeof(*argvp));
	argvp->len     = 0;
	sane_len       = (size < 2 ? 2 : size);
	argvp->argv    = (char **) mem_malloc((sane_len + 1) * sizeof(char *));
	argvp->len     = sane_len;
	argvp->argc    = 0;
	argvp->argv[0] = 0;

	argvp->push_back  = argv_push_back;
	argvp->push_front = argv_push_front;
	argvp->pop_back   = argv_pop_back;
	argvp->pop_front  = argv_pop_front;

	/* set the iterator callback */
	argvp->iter_head = argv_iter_head;
	argvp->iter_next = argv_iter_next;
	argvp->iter_tail = argv_iter_tail;
	argvp->iter_prev = argv_iter_prev;

	return argvp;
}

/* argv_add - add string to vector */

void    argv_add(ARGV *argvp,...)
{
	const char *arg;
	va_list ap;

	/* Make sure that always argvp->argc < argvp->len. */

	va_start(ap, argvp);
	while ((arg = va_arg(ap, const char *)) != 0) {
		if (SPACE_LEFT(argvp) <= 0) {
			argv_extend(argvp);
		}
		argvp->argv[argvp->argc++] = STRDUP(arg);
	}
	va_end(ap);
	argvp->argv[argvp->argc] = 0;
}

void    argv_addv(ARGV *argvp, va_list ap)
{
	const char *arg;

	/* Make sure that always argvp->argc < argvp->len. */

	while ((arg = va_arg(ap, const char *)) != 0) {
		if (SPACE_LEFT(argvp) <= 0) {
			argv_extend(argvp);
		}
		argvp->argv[argvp->argc++] = STRDUP(arg);
	}
	argvp->argv[argvp->argc] = 0;
}

/* argv_terminate - terminate string array */

void    argv_terminate(ARGV *argvp)
{
	/* Trust that argvp->argc < argvp->len. */
	argvp->argv[argvp->argc] = 0;
}

int   argv_size(ARGV *argvp)
{
	if (argvp == NULL)
		return -1;

	return argvp->argc;
}

/* argv_split - split string into token array */

ARGV *argv_split(const char *str, const char *delim)
{
	ARGV   *argvp = argv_alloc(1);
	char   *saved_string = STRDUP(str);
	char   *bp = saved_string;
	char   *arg;

	while ((arg = mystrtok(&bp, delim)) != 0)
		argv_add(argvp, arg, (char *) 0);
	argv_terminate(argvp);
	mem_free(saved_string);
	return argvp;
}
