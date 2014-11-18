#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_xinetd_cfg.h"

#endif

/* internal type define */

struct ACL_XINETD_CFG_PARSER {
	ACL_ARRAY *nv_array;
};

typedef struct ACL_XINETD_NV_PAIR {
	char *name;
/*	char *value; */
	ACL_ARRAY *values;
} ACL_XINETD_NV_PAIR;

static void __nv_pair_free(void *arg)
{
	ACL_XINETD_NV_PAIR *pair = (ACL_XINETD_NV_PAIR *) arg;

	if (pair) {
		if (pair->name)
			acl_myfree(pair->name);
		if (pair->values)
			acl_array_destroy(pair->values, acl_myfree_fn);
		acl_myfree(pair);
	}
}

static ACL_XINETD_NV_PAIR *__nv_pair_new(const char *name, const char *value)
{
	const char *myname = "__nv_pair_new";
	ACL_XINETD_NV_PAIR *pair;
	char *data;

#undef	ERETURN
#define	ERETURN(x) { \
	if (pair) { \
		if (pair->name) \
			acl_myfree(pair->name); \
		if (pair->values) \
			acl_array_destroy(pair->values, acl_myfree_fn); \
		acl_myfree(pair); \
	} \
}

	pair = (ACL_XINETD_NV_PAIR *) acl_mycalloc(1, sizeof(ACL_XINETD_NV_PAIR));
	if (pair == NULL)
		return (NULL);
	pair->name = acl_mystrdup(name);
	if (pair->name == NULL)
		ERETURN (NULL);
	data = acl_mystrdup(value);
	if (data == NULL)
		ERETURN (NULL);
	
	pair->values = acl_array_create(5);
	if (pair->values == NULL)
		ERETURN (NULL);
	if (acl_array_append(pair->values, data) < 0) {
		char  ebuf[256];
		acl_msg_error("%s(%d)->%s: acl_array_append err = %s",
				__FILE__, __LINE__, myname,
				acl_last_strerror(ebuf, sizeof(ebuf)));
		ERETURN (NULL);
	}

	return (pair);
}

static int __nv_pair_append(ACL_XINETD_NV_PAIR *pair, const char *value)
{
	const char *myname = "__nv_pair_append";
	char *data = acl_mystrdup(value);

	if (data == NULL)
		return (-1);
	if (acl_array_append(pair->values, data) < 0) {
		char  ebuf[256];

		acl_myfree(data);
		acl_msg_error("%s(%d)->%s: acl_array_append err=%s",
			__FILE__, __LINE__, myname,
			acl_last_strerror(ebuf, sizeof(ebuf)));
		return (-1);
	}
	return (0);
}

static int __xinetd_cfg_parse(ACL_XINETD_CFG_PARSER *xcp, char *line_nonl)
{
	const char *myname = "__xinetd_cfg_parse";
	char *ptr, *ptr_eq;
	char *name = NULL, *value = NULL;
	ACL_XINETD_NV_PAIR *pair, *pair_search;
	int   i, size;

	if (*line_nonl == '\0')
		return (0);

	while (*line_nonl == ' ' || *line_nonl == '\t')  /* skip ' ' and '\t' */
		line_nonl++;
	if (*line_nonl == '#') /* comment line */
		return (0);

	ptr = line_nonl;
	ptr_eq = NULL;
	while (*ptr) {
		if (*ptr == '=') {
			*ptr = 0;  /* find the position of '=' */
			ptr_eq = ptr + 1; /* make ptr_eq pointer
					   * the next char addr
					   */
			break;
		}

		ptr++;
	}

	if (ptr_eq == NULL)
		return (0);

	ptr--;  /* move before '=' */
	while (ptr > line_nonl) {  /* delete any ' ' and '\t' before '=' */
		if (*ptr != ' ' && *ptr != '\t')
			break;
		*ptr-- = 0;
	}

	/* skip any ' ' and '\t' after '=' */
	while (*ptr_eq == ' ' || *ptr_eq == '\t')
		ptr_eq++;

	if (*ptr_eq == 0) /* bugfix: by zsx, 2005.8.21 */
		return (0);

	ptr = ptr_eq;
	while (*ptr) {
		if (*ptr == '#') {
			*ptr = 0;
			break;
		}
		ptr++;
	}

	while (ptr > ptr_eq) {  /* delete any ' ' and '\t' */
		if (*ptr != ' ' && *ptr != '\t' && *ptr != '\0')
			break;
		*ptr-- = 0;
	}

	/* here line_nonl is the name */

	name = line_nonl;
	value = ptr_eq;

	pair = NULL;
	size = acl_array_size(xcp->nv_array);
	for (i = 0; i < size; i++) {
		pair_search = (ACL_XINETD_NV_PAIR *) acl_array_index(xcp->nv_array, i);
		if (pair_search == NULL)
			break;
		if (strcasecmp(name, pair_search->name) == 0) {
			pair = pair_search;
			break;
		}
	}

	/* bugfix: 2006.6.13, ---zsx */
	if (pair == NULL) { /* one new pair node */
		pair = __nv_pair_new(name, value);
		if (pair == NULL)
			ERETURN (-1);

		if (acl_array_append(xcp->nv_array, pair) < 0) {
			char  ebuf[256];
			acl_msg_error("%s(%d)->%s: acl_array_append err, serr=%s",
				__FILE__, __LINE__, myname,
				acl_last_strerror(ebuf, sizeof(ebuf)));
			return (-1);
		}
		return (0);
	} else {  /* name has already exist ! */
		return (__nv_pair_append(pair, value));
	}
}

void acl_xinetd_cfg_free(ACL_XINETD_CFG_PARSER *xcp)
{
	const char *myname = "acl_xinetd_cfg_free";

	if (xcp == NULL)
		acl_msg_fatal("%s(%d)->%s: input error",
				__FILE__, __LINE__, myname);

	if (xcp->nv_array)
		acl_array_destroy(xcp->nv_array, __nv_pair_free);
	acl_myfree(xcp);
}

ACL_XINETD_CFG_PARSER *acl_xinetd_cfg_load(const char *pathname)
{
	const char *myname = "acl_xinetd_cfg_load";
	char *content_buf = NULL;
	char *pline, *phead;
	ACL_XINETD_CFG_PARSER *xcp = NULL;
	char  ebuf[256];

#undef	RETURN
#define	RETURN(x) { \
	if (content_buf) \
		acl_myfree(content_buf); \
	return (x); \
}

#undef	ERETURN
#define	ERETURN(x) { \
	if (content_buf) \
		acl_myfree(content_buf); \
	if (xcp) { \
		if (xcp->nv_array) \
			acl_array_destroy(xcp->nv_array, __nv_pair_free); \
		acl_myfree(xcp); \
	} \
	return (x); \
}

	if (pathname == NULL || *pathname == 0) {
		acl_msg_error("%s(%d)->%s: input error",
				__FILE__, __LINE__, myname);
		ERETURN (NULL);
	}

	xcp = (ACL_XINETD_CFG_PARSER *) acl_mycalloc(1, sizeof(ACL_XINETD_CFG_PARSER));
	if (xcp == NULL) {
		acl_msg_error("%s(%d)->%s: calloc err, serr=%s",
				__FILE__, __LINE__, myname,
				acl_last_strerror(ebuf, sizeof(ebuf)));
		ERETURN (NULL);
	}

	xcp->nv_array = acl_array_create(10);
	if (xcp->nv_array == NULL) {
		acl_msg_error("%s(%d)->%s: acl_array_create err, serr=%s",
				__FILE__, __LINE__, myname,
				acl_last_strerror(ebuf, sizeof(ebuf)));
		ERETURN (NULL);
	}

	content_buf = acl_vstream_loadfile(pathname);
	if (content_buf == NULL) {
		acl_msg_error("%s(%d)->%s: load file(%s) error(%s)",
				__FILE__, __LINE__, myname, pathname,
				acl_last_strerror(ebuf, sizeof(ebuf)));
		ERETURN (NULL);
	}

	phead = content_buf;
	while (1) {
		pline = acl_mystrline(&phead);
		if (pline == NULL) {
			break;
		}

		if (__xinetd_cfg_parse(xcp, pline) < 0)
			ERETURN (NULL);
	}

	RETURN (xcp);
}

const char *acl_xinetd_cfg_get(const ACL_XINETD_CFG_PARSER *xcp, const char *name)
{
	const char *myname = "acl_xinetd_cfg_get";
	ACL_XINETD_NV_PAIR *pair;
	int   i, n;

	if (xcp == NULL || xcp->nv_array == NULL
	    || name == NULL || *name == 0) {
		acl_msg_error("%s(%d)->%s: input error",
				__FILE__, __LINE__, myname);
		return (NULL);
	}

	n = acl_array_size(xcp->nv_array);
	for (i = 0; i < n; i++) {
		pair = (ACL_XINETD_NV_PAIR *) acl_array_index(xcp->nv_array, i);
		if (pair == NULL)
			break;
		if (strcasecmp(name, pair->name) == 0)
			return (acl_array_index(pair->values, 0));
	}

	return (NULL);
}

const ACL_ARRAY *acl_xinetd_cfg_get_ex(const ACL_XINETD_CFG_PARSER *xcp, const char *name)
{
	const char *myname = "acl_xinetd_cfg_get";
	ACL_XINETD_NV_PAIR *pair;
	int   i, n;

	if (xcp == NULL || xcp->nv_array == NULL
	    || name == NULL || *name == 0) {
		acl_msg_error("%s(%d)->%s: input error",
				__FILE__, __LINE__, myname);
		return (NULL);
	}

	n = acl_array_size(xcp->nv_array);
	for (i = 0; i < n; i++) {
		pair = (ACL_XINETD_NV_PAIR *) acl_array_index(xcp->nv_array, i);
		if (pair == NULL)
			break;
		if (strcasecmp(name, pair->name) == 0)
			return (pair->values);
	}

	return (NULL);
}


int acl_xinetd_cfg_index(const ACL_XINETD_CFG_PARSER *xcp,
			int idx,
			char **ppname,
			char **ppvalue)
{
	ACL_XINETD_NV_PAIR *pair;

	if (xcp == NULL || ppname == NULL || ppvalue == NULL)
		return (-1);

	*ppname  = NULL;
	*ppvalue = NULL;

	if (xcp->nv_array == NULL)
		return (-1);

	pair = (ACL_XINETD_NV_PAIR *) acl_array_index(xcp->nv_array, idx);
	if (pair == NULL)
		return (-1);

	*ppname = pair->name;
	*ppvalue = acl_array_index(pair->values, 0);

	return (0);
}

int acl_xinetd_cfg_size(const ACL_XINETD_CFG_PARSER *xcp)
{
	if (xcp == NULL)
		return (-1);
	if (xcp->nv_array == NULL)
		return (0);
	return (acl_array_size(xcp->nv_array));
}

