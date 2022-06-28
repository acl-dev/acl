#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef ACL_UNIX
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#endif

#ifdef  ACL_WINDOWS
#include <io.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_loadcfg.h"

#endif

struct ACL_CFG_PARSER {
	ACL_ARRAY *_cfg_array;
	int    total_line;
	int    valid_line;
};

static int _cfg_file_load(ACL_FILE_HANDLE filefd, char *buf, int bsize)
{
	char *ptr;
	int   n, len;

	if (filefd < 0 || buf == NULL || bsize < 1)
		return (-1);
	ptr = buf;
	len = 0;
	bsize--;  /* make one byte room for '\0' */
	while (1) {
		n = acl_file_read(filefd, ptr, bsize, 0, NULL, NULL);
		if (n < 0)  /* read error */
			return (-1);
		else if (n == 0)  /* read over */
			break;
		ptr += n;
		len += n;
		bsize -= n;
		if (bsize <= 0) {  /* reach the end of memory area */
			buf[len] = 0;
			return (len);
		}
	}

	buf[len] = 0;

	return (len);
}

static ACL_CFG_LINE *_backup_junk_line(const char *ptr)
{
	const char myname[] = "_backup_junk_line";
	ACL_CFG_LINE *cfg_line;

	if (ptr == NULL)
		return (NULL);

	cfg_line = (ACL_CFG_LINE *) acl_mycalloc(1, sizeof(ACL_CFG_LINE));
	if (cfg_line == NULL) {
		printf("%s: calloc ACL_CFG_LINE, errmsg=%s",
				myname, acl_last_serror());
		return (NULL);
	}

	cfg_line->ncount = 0;
	cfg_line->value = NULL;
	cfg_line->pdata = acl_mystrdup(ptr);
	if (cfg_line->pdata == NULL) {
		printf("%s: strdup pdata, errmsg=%s",
				myname, acl_last_serror());
		return (NULL);
	}

	return (cfg_line);
}

static ACL_CFG_LINE *_create_cfg_line(char *data, const char *delimiter)
{
	ACL_CFG_LINE *cfg_line = NULL;
	ACL_ARRAY *a = NULL;
	int   i, n;
	char *ptr, *pdata, *pitem;

#undef	ERETURN
#define	ERETURN(x) do {                                                      \
	if (a)                                                               \
		acl_array_destroy(a, acl_myfree_fn);                         \
	if (cfg_line) {                                                      \
		if (cfg_line->value)                                         \
			acl_myfree(cfg_line);                                \
		acl_myfree(cfg_line);                                        \
	}                                                                    \
	return (x);                                                          \
} while (0);

	if (data == NULL)
		return (NULL);

	pdata = data;

	cfg_line = (ACL_CFG_LINE *) acl_mycalloc(1, sizeof(ACL_CFG_LINE));
	if (cfg_line == NULL)
		return (NULL);

	a = acl_array_create(10);
	while (1) {
		ptr = acl_mystrtok(&pdata, delimiter);
		if (ptr == NULL)
			break;
		pitem = acl_mystrdup(ptr);
		if (pitem == NULL) {
			ERETURN (NULL);
		}
		if (acl_array_append(a, (void *) pitem) < 0) {
			ERETURN (NULL);
		}
	}

	cfg_line->ncount = 0;
	cfg_line->pdata = NULL;
	n = acl_array_size(a);
	if (n > 0) {
		cfg_line->value = (char **) acl_mycalloc(1 + n, sizeof(char *));
		if (cfg_line->value == NULL) {
			ERETURN (NULL);
		}
		for (i = 0; i < n; i++) {
			pitem = (char *) acl_array_index(a, i);
			if (pitem == NULL)
				break;
			cfg_line->value[i] = pitem;
			if (cfg_line->value[i] == NULL)
				ERETURN (NULL);
			cfg_line->ncount++;
		}
	}

	/* NOTICE: in acl_array_destroy, please don't input acl_myfree, but
	 * NULL as the second parameter, because the mystrup's result
	 * set are stored in cfg_line->value now:)
	 */
	acl_array_destroy(a, NULL);

	return (cfg_line);
}

ACL_CFG_PARSER *acl_cfg_parser_load(const char *pathname, const char *delimiter)
{
	const char myname[] = "acl_cfg_parse_load";
	ACL_CFG_PARSER *parser = NULL;
	ACL_CFG_LINE *cfg_line;
	struct stat stat_buf;
	int   buf_size;
	char *content_buf = NULL, *ptr;
	char *pline_begin;
	ACL_FILE_HANDLE   filefd = ACL_FILE_INVALID;
	
#undef	ERETURN
#define	ERETURN(x) do { \
	if (content_buf != NULL) \
		acl_myfree(content_buf); \
	if (filefd != ACL_FILE_INVALID) \
		acl_file_close(filefd); \
	if (parser != NULL) { \
		acl_array_destroy(parser->_cfg_array, NULL); \
		acl_myfree(parser); \
	} \
	return (x); \
} while (0);

#undef	RETURN
#define	RETURN(x) do { \
	if (content_buf != NULL) \
		acl_myfree(content_buf); \
	if (filefd != ACL_FILE_INVALID) \
		acl_file_close(filefd); \
	return (x); \
} while (0);

	if (pathname == NULL || *pathname == 0) {
		printf("%s: invalid pathname\n", myname);
		return (NULL);
	}

	if (stat(pathname, &stat_buf) < 0) {
		printf("%s: can't stat, pathname=%s, errmsg=%s\n",
			myname, pathname, acl_last_serror());
		ERETURN (NULL);
	}

	parser = (ACL_CFG_PARSER *) acl_mycalloc(1, sizeof(*parser));
	if (parser == NULL) {
		printf("%s: can't calloc ACL_CFG_PARSER, pathname=%s, errmsg=%s",
			myname, pathname, acl_last_serror());
		ERETURN (NULL);
	}

	parser->_cfg_array = acl_array_create(10);
	if (parser->_cfg_array == NULL) {
		printf("%s: can't create array, pathname=%s, errmsg=%s",
			myname, pathname, acl_last_serror());
		ERETURN (NULL);
	}
	parser->total_line = 0;
	parser->valid_line = 0;

	buf_size = (int) stat_buf.st_size + 256;
	content_buf = (char *) acl_mycalloc(1, buf_size);
	if (content_buf == NULL) {
		printf("%s: can't calloc, pathname=%s, errmsg=%s\n",
			myname, pathname, acl_last_serror());
		ERETURN (NULL);
	}
	
#ifdef ACL_UNIX
# ifdef ACL_ANDROID
	filefd = acl_file_open(pathname, O_RDWR, 0644);
# else
	filefd = acl_file_open(pathname, O_RDWR, S_IREAD | S_IWRITE | S_IRGRP);
# endif
#elif defined(ACL_WINDOWS)
	filefd = acl_file_open(pathname, O_RDWR, S_IREAD | S_IWRITE);
#else
# error "unknown OS"
#endif

	if (filefd == ACL_FILE_INVALID) {
		printf("%s: can't open, pathname=%s, errmsg=%s\n",
			myname, pathname, acl_last_serror());

		ERETURN (NULL);
	}

	if (_cfg_file_load(filefd, content_buf, buf_size) < 0) {
		printf("%s: can't read, pathname=%s, errmsg=%s\n",
			myname, pathname, acl_last_serror());
		ERETURN (NULL);
	}

#undef	SKIP
#define SKIP(var, cond) \
        for (; *var && (cond); var++) {}

	ptr = content_buf;
	while (*ptr) {
		pline_begin = ptr;  /* keep the line header */
		/* first, skip all ' ' and '\t' */
		SKIP(ptr, (*ptr == ' ' || *ptr == '\t'));

		if  (*ptr == '#') {  /* the comment line */
			SKIP(ptr, *ptr != '\n'); /* find the line's end */
			if (*ptr) {  /* this must be '\n' */
				*ptr++ = 0;  /* set '\0' and skip one byte */
			}

			cfg_line = _backup_junk_line(pline_begin);
			if (cfg_line == NULL)
				ERETURN (NULL);
			if (acl_array_append(parser->_cfg_array,
					(void *) cfg_line) < 0) {
				printf("%s: can't add ACL_CFG_LINE to array, "
					"errmsg=%s", myname, acl_last_serror());
				ERETURN (NULL);
			}
			parser->total_line++;
			cfg_line->line_number = parser->total_line;
			continue;
		} else if (*ptr == '\r' || *ptr == '\n') {
			/* SKIP(ptr, (*ptr == '\r' || *ptr == '\n')); */
			if (*ptr == '\r' && *(ptr + 1) == '\n') {
				*ptr = 0; /* set '\0' first and go on */
				ptr += 2;
			} else if (*ptr == '\n') {
				*ptr = 0; /* set '\0' first and go on */
				ptr++;
			}

			cfg_line = _backup_junk_line(pline_begin);
			if (cfg_line == NULL)
				ERETURN (NULL);
			if (acl_array_append(parser->_cfg_array,
					(void *) cfg_line) < 0) {
				printf("%s: can't add ACL_CFG_LINE to array, "
					"errmsg=%s", myname, acl_last_serror());
				ERETURN (NULL);
			}
			parser->total_line++;
			cfg_line->line_number = parser->total_line;
			continue;
		}

		pline_begin = ptr;  /* reset the line header */

		/* find the line's end */
		SKIP(ptr, (*ptr != '\n' && *ptr != '\r'));
		if (*ptr) {  /* this must be '\r' or '\n' */
			if (*ptr == '\r' && *(ptr + 1) == '\n') {
				*ptr = 0; /* set '\0' first and go on */
				ptr += 2;
			} else if (*ptr == '\n') {
				*ptr = 0; /* set '\0' first and go on */
				ptr++;
			}
		}

		/* make ptr to the next line's beginning */
		/* SKIP(ptr, (*ptr == '\r' || *ptr == '\n')); */

		cfg_line = _create_cfg_line(pline_begin, delimiter);
		if (cfg_line == NULL)
			ERETURN (NULL);
		if (acl_array_append(parser->_cfg_array, (void *) cfg_line) < 0) {
			printf("%s: can't add ACL_CFG_LINE to array, errmsg=%s",
				myname, acl_last_serror());
			ERETURN (NULL);
		}
		parser->total_line++;
		parser->valid_line++;
		cfg_line->line_number = parser->total_line;
	}

	if (parser->total_line != acl_array_size(parser->_cfg_array)) {
		printf("%s: total_line=%d, acl_array_size=%d, errmsg=not equal\n",
			myname, parser->total_line,
			acl_array_size(parser->_cfg_array));
	}

	RETURN (parser);
#ifdef ACL_BCB_COMPILER
	return (NULL);
#endif
}

static void _cfg_line_free(void *arg)
{
	ACL_CFG_LINE *cfg_line;
	int   i;

	cfg_line = (ACL_CFG_LINE *) arg;

	if (cfg_line == NULL)
		return;

	for (i = 0; i < cfg_line->ncount; i++) {
		if (cfg_line->value == NULL)
			break;
		if (cfg_line->value[i] == NULL)
			break;
		acl_myfree(cfg_line->value[i]);
	}
	if (cfg_line->value != NULL)
		acl_myfree(cfg_line->value);
	if (cfg_line->pdata != NULL)
		acl_myfree(cfg_line->pdata);
	acl_myfree(cfg_line);
}

void acl_cfg_parser_free(ACL_CFG_PARSER *parser)
{
	if (parser) {
		if (parser->_cfg_array)
			acl_array_destroy(parser->_cfg_array, _cfg_line_free);
		acl_myfree(parser);
	}
}

void acl_cfg_parser_walk(ACL_CFG_PARSER *parser, ACL_CFG_WALK_FN walk_fn)
{
	int   i, n;
	ACL_CFG_LINE *cfg_line;

	if (parser) {
		n = acl_array_size(parser->_cfg_array);
		for (i = 0; i < n; i++) {
			cfg_line = (ACL_CFG_LINE *)
				acl_array_index(parser->_cfg_array, i);
			if (cfg_line == NULL)
				break;
			walk_fn((void *) cfg_line);
		}
	}
}

int acl_cfg_line_replace(ACL_CFG_LINE *cfg_line, const char **value,
	int from, int to)
{
	const char myname[] = "acl_cfg_line_replace";
	int   i, n, j;

	if (cfg_line == NULL)
	       return (-1);	

	n = cfg_line->ncount;

	if (from >= n)
		from = n - 1;
	if (to >= n)
		to = n - 1;

	if (from < 0 || to < 0)
		return (-1);

	n = 0;
	j = 0;
	if (from <= to) {
		for (i = from; i <= to; i++) {
			if (cfg_line->value == NULL)
				break;
			if ((const char **) cfg_line->value == value)
				break;
			if (cfg_line->value[i] == value[j]) {
				j++;
				continue;
			}
			if (cfg_line->value[i] != NULL)
				acl_myfree(cfg_line->value[i]);
			cfg_line->value[i] = acl_mystrdup(value[j]);
			if (cfg_line->value[i] == NULL) {
				printf("%s: can't strdup, errmsg=%s\n",
					myname, acl_last_serror());
				exit (1);
			}
			j++;
			n++;
		}
	} else {
		for (i = from; i >= to; i--) {
			if ((const char **) cfg_line->value == value)
				break;
			if (cfg_line->value[i] == value[j]) {
				j++;
				continue;
			}
			if (cfg_line->value[i] != NULL)
				acl_myfree(cfg_line->value[i]);
			cfg_line->value[i] = acl_mystrdup(value[j]);
			if (cfg_line->value[i] == NULL) {
				printf("%s: can't strdup, errmsg=%s\n",
					myname, acl_last_serror());
				exit (1);
			}
			j++;
			n++;
		}
	}

	return (n);
}

ACL_CFG_LINE *acl_cfg_parser_index(const ACL_CFG_PARSER *parser, int idx)
{
	ACL_CFG_LINE *cfg_line;

	if (parser == NULL || idx < 0)
		return (NULL);

	cfg_line = (ACL_CFG_LINE *) acl_array_index(parser->_cfg_array, idx);
	return (cfg_line);
}

int acl_cfg_parser_size(const ACL_CFG_PARSER *parser)
{
	if (parser == NULL)
		return (-1);

	return (parser->total_line);
}

static int _cfg_line_dump(ACL_FILE_HANDLE filefd, const ACL_CFG_LINE *cfg_line,
	const char *delimiter)
{
	const char myname[] = "_cfg_line_dump";
	char *pbuf, *ptr;
	int   dlen = 0, i, j,  n;

	n = cfg_line->ncount;
	if (delimiter != NULL)
		j = (int) strlen(delimiter);
	else
		j = 0;

	if (cfg_line->value != NULL) {
		for (i = 0; i < n; i++) {
			if (cfg_line->value[i] == NULL)
				break;
			dlen += (int) strlen(cfg_line->value[i]) + j;
		}
		dlen += 2;   /* for '\n' and '\0' */
		pbuf = (char *) acl_mycalloc(1, dlen);
		ptr = pbuf;
		for (i = 0; i < n; i++) {
			if (cfg_line->value[i] == NULL)
				break;
			if (i < n -1)
				sprintf(ptr, "%s%s", cfg_line->value[i], delimiter);
			else
				sprintf(ptr, "%s", cfg_line->value[i]);
			ptr = ptr + strlen(ptr);
		}
		ptr = ptr + strlen(ptr);
		strcat(ptr, "\n\0");
		i = acl_file_write(filefd, pbuf, strlen(pbuf), 0, NULL, NULL);
		if (i <= 0) {
			printf("%s: can't write pbuf, error=%s\n",
				myname, acl_last_serror());
			acl_myfree(pbuf);
			return (-1);
		}
	} else if (cfg_line->pdata != NULL) {
		dlen = (int) strlen(cfg_line->pdata) + 2;
		pbuf = (char *) acl_mycalloc(1, dlen);
		if (pbuf == NULL)
			return (-1);
		sprintf(pbuf, "%s\n", cfg_line->pdata);
		i = acl_file_write(filefd, pbuf, strlen(pbuf), 0, NULL, NULL);
		if (i <= 0)
			return (-1);
	}

	return (0);
}

int acl_cfg_parser_dump(const ACL_CFG_PARSER *parser,
			const char *pathname,
			const char *delimiter)
{
	const char myname[] = "acl_cfg_parser_dump";
	ACL_CFG_LINE *cfg_line;
	ACL_FILE_HANDLE filefd = ACL_FILE_INVALID;
	int   i, n, ret;

#undef	RETURN
#define	RETURN(x) do { \
	if (filefd != ACL_FILE_INVALID) \
		acl_file_close(filefd); \
	return (x); \
} while (0);

	if (parser == NULL || pathname == NULL || *pathname == 0)
		return (-1);
#ifdef ACL_UNIX
# ifdef ACL_ANDROID
	filefd = acl_file_open(pathname,
			O_CREAT | O_TRUNC | O_APPEND | O_WRONLY, 0644);
# else
	filefd = acl_file_open(pathname,
			O_CREAT | O_TRUNC | O_APPEND | O_WRONLY,
			S_IREAD | S_IWRITE | S_IRGRP);
# endif
#elif defined(ACL_WINDOWS)
	filefd = acl_file_open(pathname,
		O_CREAT | O_TRUNC | O_APPEND | O_WRONLY,
		S_IREAD | S_IWRITE);
#else
# error "unknown OS"
#endif
	if (filefd == ACL_FILE_INVALID) {
		printf("%s: can't open, pathname=%s, errmsg=%s\n",
			myname, pathname, acl_last_serror());
		RETURN(-1);
	}

	n = acl_array_size(parser->_cfg_array);
	for (i = 0; i < n; i++) {
		cfg_line = (ACL_CFG_LINE *)
			acl_array_index(parser->_cfg_array, i);
		if (cfg_line == NULL)
			break;
		ret = _cfg_line_dump(filefd, cfg_line, delimiter);
		if (ret < 0) {
			RETURN (-1);
		}
	}

	RETURN (0);
#ifdef ACL_BCB_COMPILER
	return (0);
#endif
}

int acl_cfg_parser_append(ACL_CFG_PARSER *parser, ACL_CFG_LINE *cfg_line)
{
	const char myname[] = "acl_cfg_parser_append";

	if (parser == NULL || cfg_line == NULL) {
		printf("%s: input error\n", myname);
		return (-1);
	}

	if (parser->_cfg_array == NULL) {
		parser->_cfg_array = acl_array_create(10);
		if (parser->_cfg_array == NULL) {
			printf("%s: can't create array, errmsg=%s",
					myname, acl_last_serror());
			return (-1);
		}
		parser->total_line = 0;
		parser->valid_line = 0;
	}

	if (acl_array_append(parser->_cfg_array, (void *) cfg_line) < 0) {
		printf("%s: can't add ACL_CFG_LINE to array, errmsg=%s",
			myname, acl_last_serror());
		return (-1);
	}
	parser->total_line++;
	if (cfg_line->pdata == NULL && cfg_line->value != NULL)
		parser->valid_line++;
	cfg_line->line_number = parser->total_line;

	return (0);
}

int acl_cfg_parser_delete(ACL_CFG_PARSER *parser, const char *name)
{
	const char myname[] = "acl_cfg_parser_delete";
	ACL_CFG_LINE *cfg_line = NULL;
	int   i, n, ok = 0, j;

	if (parser == NULL || name == NULL || *name == 0) {
		printf("%s: input error\n", myname);
		return (-1);
	}

	if (parser->_cfg_array == NULL) {
		return (0);
	}

	n = acl_array_size(parser->_cfg_array);
	for (i = 0; i < n; i++) {
		cfg_line = (ACL_CFG_LINE *) acl_array_index(parser->_cfg_array, i);
		if (cfg_line == NULL)
			return (0);
		if (cfg_line->ncount < 1)
			continue;
		if (strcmp(cfg_line->value[0], name) == 0) {
			ok = 1;
			break;
		}
	}

	if (ok) {
		if (cfg_line->pdata == NULL && cfg_line->value != NULL)
			parser->valid_line--;
		parser->total_line--;
		acl_array_delete_idx(parser->_cfg_array, i, _cfg_line_free);

		n = acl_array_size(parser->_cfg_array);
		for (j = i; j < n; j++) {
			cfg_line = (ACL_CFG_LINE *) acl_array_index(parser->_cfg_array, j);
			if (cfg_line == NULL)
				break;
			cfg_line->line_number--;
		}
	}

	return (0);
}

ACL_CFG_LINE *acl_cfg_line_new(const char **value, int ncount)
{
	ACL_CFG_LINE *cfg_line = NULL;
	int   i;

#undef	ERETURN
#define	ERETURN(x) do { \
	if (cfg_line == NULL) \
		return (x); \
	if (cfg_line->value == NULL) { \
		acl_myfree(cfg_line); \
		return (x); \
	} \
	for (i = 0; i < cfg_line->ncount; i++) { \
		if (cfg_line->value[i] == NULL) \
			break; \
		acl_myfree(cfg_line->value[i]); \
	} \
	acl_myfree(cfg_line->value); \
	acl_myfree(cfg_line); \
	return (x); \
} while (0);

	if (value == NULL || ncount <= 0)
		return (NULL);

	cfg_line = (ACL_CFG_LINE *) acl_mycalloc(1, sizeof(ACL_CFG_LINE));
	if (cfg_line == NULL)
		return (NULL);

	cfg_line->value = (char **) acl_mycalloc(1 + ncount, sizeof(char *));
	if (cfg_line->value == NULL)
		ERETURN (NULL);

	cfg_line->pdata = NULL;
	cfg_line->ncount = 0;
	cfg_line->line_number = 0;

	for (i = 0; i < ncount; i++) {
		cfg_line->value[i] = acl_mystrdup(value[i]);
		if (cfg_line->value[i] == NULL)
			ERETURN (NULL);
		cfg_line->ncount++;
	}

	return (cfg_line);
}

