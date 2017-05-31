#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#ifdef	ACL_UNIX
#include <unistd.h>
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_process.h"
#include "thread/acl_thread.h"

#endif

#define	BUF_SIZE	4096

static char *get_tls_buf(void)
{
	const char *myname = "get_tls_buf";
	static acl_pthread_key_t buf_key = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
	char *buf;
	static char  buf_unsafe[BUF_SIZE];

	buf = (char*) acl_pthread_tls_get(&buf_key);
	if (buf != NULL)
		return (buf);

	if (buf_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
		acl_msg_warn("%s(%d): acl_pthread_tls_get error(%s), "
			"use unsafe buf", myname, __LINE__, acl_last_serror());
		buf = buf_unsafe;
	} else {
		buf = (char*) acl_mycalloc(1, BUF_SIZE);
		acl_pthread_tls_set(buf_key, buf,
			(void (*)(void*)) acl_myfree_fn);
	}
	return (buf);
}

#ifdef	ACL_LINUX
const char *acl_process_path()
{
	const char *myname = "acl_process_path";
	char *buf_ptr = get_tls_buf();
	int   ret;

	ret = readlink("/proc/self/exe", buf_ptr, BUF_SIZE);
	if (ret < 0) {
		acl_msg_error("%s(%d): readlink error(%s)",
			myname, __LINE__, acl_last_serror());
		return (NULL);
	}
	return (buf_ptr);
}

const char *acl_getcwd()
{
	const char *myname = "acl_getcwd";
	char *buf_ptr = get_tls_buf();
	char *ptr;

	ptr = getcwd(buf_ptr, BUF_SIZE);
	if (ptr == NULL) {
		acl_msg_error("%s(%d): getcwd error(%s)",
			myname, __LINE__, acl_last_serror());
	}
	return (ptr);
}
#elif	defined(ACL_FREEBSD)
const char *acl_process_path()
{
	const char *myname = "acl_process_path";
	char *buf_ptr = get_tls_buf();
	int   ret;

	ret = readlink("/proc/curproc/file", buf_ptr, BUF_SIZE);
	if (ret < 0) {
		acl_msg_error("%s(%d): readlink error(%s)",
			myname, __LINE__, acl_last_serror());
		return (NULL);
	}
	return (buf_ptr);
}

const char *acl_getcwd()
{
	const char *myname = "acl_getcwd";
	char *buf_ptr = get_tls_buf();
	char *ptr;

	ptr = getcwd(buf_ptr, BUF_SIZE);
	if (ptr == NULL) {
		acl_msg_error("%s(%d): getcwd error(%s)",
			myname, __LINE__, acl_last_serror());
	}
	return (ptr);
}
#elif	defined(ACL_MACOSX)
const char *acl_process_path()
{
	const char *myname = "acl_process_path";
	char *buf_ptr = get_tls_buf();
	ssize_t   ret;

	ret = readlink("/proc/curproc/file", buf_ptr, BUF_SIZE);
	if (ret < 0) {
		acl_msg_error("%s(%d): readlink error(%s)",
			myname, __LINE__, acl_last_serror());
		return (NULL);
	}
	return (buf_ptr);
}

const char *acl_getcwd()
{
	const char *myname = "acl_getcwd";
	char *buf_ptr = get_tls_buf();
	char *ptr;

	ptr = getcwd(buf_ptr, BUF_SIZE);
	if (ptr == NULL) {
		acl_msg_error("%s(%d): getcwd error(%s)",
			myname, __LINE__, acl_last_serror());
	}
	return (ptr);
}

#elif	defined(ACL_SUNOS5)
const char *acl_process_path()
{
	const char *myname = "acl_process_path";
	char *buf_ptr = get_tls_buf();
	int   ret;

	ret = readlink("/proc/self/path/a.out", buf_ptr, BUF_SIZE);
	if (ret < 0) {
		acl_msg_error("%s(%d): readlink error(%s)",
				myname, __LINE__, acl_last_serror());
		return (NULL);
	}
	return (buf_ptr);
}

#if 0
#include <stdlib.h>
const char *acl_process_path()
{
	const char *myname = "acl_process_path";
	const char *ptr;

	ptr = getexecname();
	if (ptr == NULL) {
		acl_msg_error("%s(%d): readlink error(%s)",
				myname, __LINE__, acl_last_serror());
		return (NULL);
	}
	return (ptr);
}
#endif

const char *acl_getcwd()
{
	const char *myname = "acl_getcwd";
	char *buf_ptr = get_tls_buf();
	char *ptr;

	ptr = getcwd(buf_ptr, BUF_SIZE);
	if (ptr == NULL) {
		acl_msg_error("%s(%d): getcwd error(%s)",
			myname, __LINE__, acl_last_serror());
	}
	return (ptr);
}
#elif	defined(ACL_WINDOWS)
#include <direct.h>

const char *acl_process_path()
{
	const char *myname = "acl_process_path";
	char *buf_ptr = get_tls_buf();
	int   ret;

	ret = GetModuleFileName(NULL, buf_ptr, BUF_SIZE);
	if (ret == 0) {
		acl_msg_error("%s(%d): readlink error(%s)",
			myname, __LINE__, acl_last_serror());
		return (NULL);
	}
	return (buf_ptr);
}

const char *acl_getcwd()
{
	const char *myname = "acl_getcwd";
	char *buf_ptr = get_tls_buf();
	char *ptr;

	ptr = _getcwd(buf_ptr, BUF_SIZE);
	if (ptr == NULL) {
		acl_msg_error("%s(%d): getcwd error(%s)",
			myname, __LINE__, acl_last_serror());
	}
	return (ptr);
}
#else
# error "unknown OS type"
#endif
