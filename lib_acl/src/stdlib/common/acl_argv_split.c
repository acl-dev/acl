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

ACL_ARGV *acl_argv_split(const char *str, const char *delim) {
	return acl_argv_split3(str, delim, NULL);
}

ACL_ARGV *acl_argv_split3(const char *str, const char *delim,
		ACL_DBUF_POOL *dbuf) {
	ACL_ARGV *argvp = acl_argv_alloc2(5, dbuf);
	char   *saved_string =
		dbuf ? acl_dbuf_pool_strdup(dbuf, str) : acl_mystrdup(str);
	char   *bp = saved_string;
	char   *arg;

	while ((arg = acl_mystrtok(&bp, delim)) != 0) {
		acl_argv_add(argvp, arg, (char *) 0);
	}
	acl_argv_terminate(argvp);
	if (dbuf) {
		acl_dbuf_pool_free(dbuf, saved_string);
	} else {
		acl_myfree(saved_string);
	}
	return argvp;
}

/* acl_argv_splitn - split string into token array with max items */

ACL_ARGV *acl_argv_splitn(const char *str, const char *delim, size_t n) {
	return acl_argv_splitn4(str, delim, n, NULL);
}

ACL_ARGV *acl_argv_splitn4(const char *str, const char *delim,
		size_t n, ACL_DBUF_POOL *dbuf) {
	ACL_ARGV *argvp = acl_argv_alloc2(n > 0 ? (int) n : 1, dbuf);
	char   *saved_string = dbuf ?
		acl_dbuf_pool_strdup(dbuf, str) : acl_mystrdup(str);
	char   *bp = saved_string;
	char   *arg;

	while (n-- > 0 && (arg = acl_mystrtok(&bp, delim)) != 0) {
		acl_argv_add(argvp, arg, (char *) 0);
	}
	acl_argv_terminate(argvp);
	if (dbuf) {
		acl_dbuf_pool_free(dbuf, saved_string);
	} else {
		acl_myfree(saved_string);
	}
	return argvp;
}

/* acl_argv_split_append - split string into token array, append to array */

ACL_ARGV *acl_argv_split_append(ACL_ARGV *argvp, const char *str,
		const char *delim) {
	char   *saved_string = argvp->dbuf ?
		acl_dbuf_pool_strdup(argvp->dbuf, str) : acl_mystrdup(str);
	char   *bp = saved_string;
	char   *arg;

	while ((arg = acl_mystrtok(&bp, delim)) != 0) {
		acl_argv_add(argvp, arg, (char *) 0);
	}
	acl_argv_terminate(argvp);
	if (argvp->dbuf) {
		acl_dbuf_pool_free(argvp->dbuf, saved_string);
	} else {
		acl_myfree(saved_string);
	}
	return argvp;
}

/* acl_argv_splitn_append - split string into token array, append to
 * array with max items */

ACL_ARGV *acl_argv_splitn_append(ACL_ARGV *argvp, const char *str,
		const char *delim, size_t n) {
	char   *saved_string = argvp->dbuf ?
		acl_dbuf_pool_strdup(argvp->dbuf, str) : acl_mystrdup(str);
	char   *bp = saved_string;
	char   *arg;

	while (n-- > 0 && (arg = acl_mystrtok(&bp, delim)) != 0) {
		acl_argv_add(argvp, arg, (char *) 0);
	}
	acl_argv_terminate(argvp);
	if (argvp->dbuf) {
		acl_dbuf_pool_free(argvp->dbuf, saved_string);
	} else {
		acl_myfree(saved_string);
	}
	return argvp;
}

ACL_ARGV *acl_argv_quote_split(const char *str, const char *delim) {
	return acl_argv_quote_split4(str, delim, NULL);
}

ACL_ARGV *acl_argv_quote_split4(const char *str, const char *delim,
	ACL_DBUF_POOL *dbuf) {
	ACL_ARGV *argvp = acl_argv_alloc2(1, dbuf);
	int     ch, quote = 0, backslash = 0;
	ACL_VSTRING *buf = acl_vstring_dbuf_alloc(dbuf, 128);

#define	ADDCH	ACL_VSTRING_ADDCH
#define	TERM	ACL_VSTRING_TERMINATE
#define	LEN	ACL_VSTRING_LEN
#define	STR	acl_vstring_str
#define	RESET	ACL_VSTRING_RESET

	while ((ch = (unsigned char) *str) != 0) {
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

	if (LEN(buf) > 0) {
		acl_argv_add(argvp, STR(buf), NULL);
	}

	acl_argv_terminate(argvp);
	acl_vstring_free(buf);
	return argvp;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct ARGV_VIEW {
	ACL_ARGV_VIEW view;
	char *args[50];
	char  buf[1];
} ARGV_VIEW;

static void *argv_view_iter_head(ACL_ITER *iter, const ACL_ARGV_VIEW *view) {
	return view->argv.iter_head(iter, &view->argv);
}

static void *argv_view_iter_next(ACL_ITER *iter, const ACL_ARGV_VIEW *view) {
	return view->argv.iter_next(iter, &view->argv);
}

static void *argv_view_iter_tail(ACL_ITER *iter, const ACL_ARGV_VIEW *view) {
	return view->argv.iter_tail(iter, &view->argv);
}

static void *argv_view_iter_prev(ACL_ITER *iter, const ACL_ARGV_VIEW *view) {
	return view->argv.iter_prev(iter, &view->argv);
}

static void argv_extend(ARGV_VIEW *view) {
	view->view.argv.len = view->view.argv.len * 10;
	if (view->view.argv.argv == view->args) {
		view->view.argv.argv = (char **)
			acl_mymalloc(view->view.argv.len * sizeof(char *));
		memcpy(view->view.argv.argv, view->args,
			view->view.argv.argc * sizeof(char *));
	} else {
		view->view.argv.argv = (char **)
			acl_myrealloc((char *) view->view.argv.argv,
				(view->view.argv.len + 1) * sizeof(char *));
	}
}

#define SPACE_LEFT(a) ((a)->len - (a)->argc - 1)

ACL_ARGV_VIEW *acl_argv_view_split(const char *str, const char *delim) {
	size_t len = strlen(str);
	ARGV_VIEW *view = (ARGV_VIEW*) acl_mymalloc(sizeof(ARGV_VIEW) + len + 1);
	char *ptr = view->buf, *start = ptr;

	memcpy(ptr, str, len);
	ptr[len] = 0;

	view->view.argv.dbuf = NULL;
	view->view.argv.argc = 0;
	view->view.argv.len  = sizeof(view->args) / sizeof(view->args[0]);
	view->view.argv.argv = view->args;

	/* set the iterator callback */
	acl_argv_iter_init(&view->view.argv);
	view->view.iter_head = argv_view_iter_head;
	view->view.iter_next = argv_view_iter_next;
	view->view.iter_tail = argv_view_iter_tail;
	view->view.iter_prev = argv_view_iter_prev;

	while (*ptr != 0) {
#if 1
		if (strchr(delim, *ptr) == NULL) {
			ptr++;
			continue;
		}

		if (start < ptr) {
			if (SPACE_LEFT(&view->view.argv) <= 0) {
				argv_extend(view);
			}
			*ptr = 0;
			view->view.argv.argv[view->view.argv.argc++] = start;
		}
		start = ptr + 1;
		ptr++;
#else
		if (strchr(delim, *ptr) != NULL) {
			if (start < ptr) {
				if (SPACE_LEFT(&view->view.argv) <= 0) {
					argv_extend(view);
				}
				*ptr = 0;
				view->view.argv.argv[view->view.argv.argc++] = start;
			}
			start = ptr + 1;
		}
		ptr++;
#endif
	}

	if (*start) {
		if (SPACE_LEFT(&view->view.argv) <= 0) {
			argv_extend(view);
		}
		view->view.argv.argv[view->view.argv.argc++] = start;
	}
	return (ACL_ARGV_VIEW *) view;
}

void acl_argv_view_free(ACL_ARGV_VIEW *view) {
	if (view != NULL) {
		if (view->argv.argv != ((ARGV_VIEW*) view)->args) {
			acl_myfree(view->argv.argv);
		}
		acl_myfree(view);
	}
}
