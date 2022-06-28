#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "net/acl_access.h"

#endif

/* local variables */
static ACL_IPLINK *__host_allow_link = NULL;
static void (*__log_fn) (const char *fmt, ...) = acl_msg_info;
static int __host_allow_all = 0;

static void __access_init(void)
{
	const char *myname = "__access_init";

	if (__host_allow_link)
		return;

	__host_allow_link = acl_iplink_create(10);
	if (__host_allow_link == NULL) {
		char tbuf[256];
		__log_fn("FATAL, %s(%d)->%s: call acl_iplink_create error=%s",
				__FILE__, __LINE__, myname,
				acl_last_strerror(tbuf, sizeof(tbuf)));
		exit (1);
	}
}

int acl_access_add(const char *data, const char *sep1, const char *sep2)
{
	/* data format: ip1 sep2 ip2 sep1 ip3 sep2 ip4 ...
	 * example:
	 * 127.0.0.1:127.0.0.1, 10.0.250.1:10.0.250:10, 192.168.0.1:192.168.0.255
	 * 127.0.0.1,127.0.0.1; 10.0.250.1,10.0.250:10; 192.168.0.1,192.168.0.255
	 */
	const char *myname = "acl_access_add";
	ACL_ARGV *items;
	char *psrc, *ptr, *from, *to, buf[256];
	int   i;

	if (data == NULL || *data == 0) {
		__log_fn("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return (0);
	}

	if (__host_allow_all)
		return (0);

	if (strcasecmp(data, "all") == 0) {
		__host_allow_all = 1;
		return (0);
	}

	if (strcmp(sep1, "0.0.0.0") == 0 && strcmp(sep2, "255.255.255.255") == 0) {
		__host_allow_all = 1;
		return (0);
	}

	if (__host_allow_link == NULL)
		__access_init();

	items = acl_argv_split(data, sep1);
	if (items == NULL) {
		char tbuf[256];
		__log_fn("%s, %s(%d): acl_argv_split error(%s)",
			__FILE__, myname, __LINE__, acl_last_strerror(tbuf, sizeof(tbuf)));
		exit (1);
	}

#define	STRIP_SPACE(_ptr) do {  \
	char *_tmp;  \
	while (*_ptr == ' ' || *_ptr == '\t')  \
		_ptr++;  \
	_tmp = _ptr;  \
	while (*_tmp) {  \
		if (*_tmp == ' ' || *_tmp == '\t') {  \
			*_tmp = 0;  \
			break;  \
		}  \
		_tmp++;  \
	}  \
} while (0)

	for (i = 0; i < items->argc; i++) {
		psrc = acl_argv_index(items, i);
		ACL_SAFE_STRNCPY(buf, psrc, sizeof(buf) - 1);
		ptr = buf;

		from = acl_mystrtok(&ptr, sep2);
		if (from == NULL || *from == 0) {
			__log_fn("%s, %s(%d): invalid data(%s)",
				__FILE__, myname, __LINE__, psrc);
			continue;
		}

		STRIP_SPACE(from);
		if (*from == 0)
			continue;

		to = acl_mystrtok(&ptr, sep2);
		if (to == NULL || *to == 0) {
			__log_fn("%s, %s(%d): invalid data(%s)",
				__FILE__, myname, __LINE__, psrc);
			continue;
		}

		STRIP_SPACE(to);
		if (*to == 0)
			continue;

		if (acl_msg_verbose)
			__log_fn("add access: from(%s), to(%s)", from, to);
		if (acl_iplink_insert(__host_allow_link, from, to) == NULL)
			__log_fn("%s, %s(%d): acl_iplink_insert error(%s)",
				__FILE__, myname, __LINE__, psrc);
	}

	acl_argv_free(items);

	return (0);
}

int acl_access_cfg(ACL_XINETD_CFG_PARSER *xcp, const char *name)
{
	const char *myname = "acl_access_cfg";
	const ACL_ARRAY *p_array;
	const char *pctr;
	int   i, n;

	p_array = acl_xinetd_cfg_get_ex(xcp, name);
	if (p_array == NULL) {
		__log_fn("%s(%d)->%s: host allow all",
				__FILE__, __LINE__, myname);
		return (0);
	}

	n = acl_array_size(p_array);
	for (i = 0; i < n; i++) {
		pctr = (const char *) acl_array_index(p_array, i);
		if (pctr == NULL)
			break;
		acl_access_add(pctr, ",", ":");
	}

	return (0);
} 

void acl_access_setup_logfn(void (*log_fn)(const char *fmt, ...))
{
	if (log_fn != NULL)
		__log_fn = log_fn;
}

int acl_access_permit(const char *addr)
{
	char  ip[32], *ptr;

	if (__host_allow_all)
		return (1);
	if (__host_allow_link == NULL)
		return (1);

	ACL_SAFE_STRNCPY(ip, addr, sizeof(ip));
	ptr = strchr(ip, ':');
	if (ptr)
		*ptr = 0;
	if (acl_iplink_lookup_str(__host_allow_link, ip) != NULL)
		return (1);

	return (0);
}

static void __access_cfg_out(void)
{

	acl_iplink_list(__host_allow_link);
}

void acl_access_debug(void)
{
	__access_cfg_out();
}
