#include "lib_acl.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#ifdef	ACL_UNIX
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include "test_stdlib.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

static ACL_FILE_HANDLE  __file_fd = ACL_FILE_INVALID;

static ACL_FILE_HANDLE __mylock_open(const char *filename)
{
	ACL_FILE_HANDLE   fd;
	int   flag = O_WRONLY|O_APPEND|O_CREAT;
#ifdef ACL_UNIX
	int   mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
#else
	int   mode = 0600;
#endif

	fd = acl_file_open(filename, flag, mode);
	return (fd);
}

static ACL_FILE_HANDLE __mylock_try_open(const char *filename)
{
	char  myname[] = "__mylock_try_open";

	if (__file_fd < 0) {
		if (filename == NULL || *filename == 0)
			acl_msg_fatal("%s(%d): filename invalid", myname, __LINE__);
		__file_fd = __mylock_open(filename);
	}
	if (__file_fd < 0)
		acl_msg_fatal("%s(%d): open file(%s) error(%s)",
				myname, __LINE__, filename, strerror(errno));

	return (__file_fd);
}

static int __mylock_op(ACL_FILE_HANDLE fd, int op, char *errbuf, int size)
{
	int   lock_style = ACL_FLOCK_STYLE_FCNTL;
	int   ret;

	ret = acl_myflock(fd, lock_style, op);
	if (ret < 0) {
		if (errbuf && size > 0)
			snprintf(errbuf, size, "flock error(%s)", strerror(errno));
		return (-1);
	}

	return (0);
}

int test_mylock_unlock(AUT_LINE *test_line, void *arg acl_unused)
{
	int   operation = ACL_FLOCK_OP_NONE;
	const char *filename;
	ACL_FILE_HANDLE   fd;

	AUT_SET_STR(test_line, "file", filename);
	fd = __mylock_try_open(filename);
	return (__mylock_op(fd, operation, NULL, 0));
}

int test_mylock_excl(AUT_LINE *test_line, void *arg acl_unused)
{
	int   operation = ACL_FLOCK_OP_EXCLUSIVE;
	const char *filename;
	ACL_FILE_HANDLE   fd;

	AUT_SET_STR(test_line, "file", filename);
	fd = __mylock_try_open(filename);
	return (__mylock_op(fd, operation, NULL, 0));
}

int test_mylock_nowait(AUT_LINE *test_line, void *arg acl_unused)
{
	int   operation = ACL_FLOCK_OP_NOWAIT;
	const char *filename;
	ACL_FILE_HANDLE   fd;

	AUT_SET_STR(test_line, "file", filename);
	fd = __mylock_try_open(filename);
	return (__mylock_op(fd, operation, NULL, 0));
}

int test_mylock_shared(AUT_LINE *test_line, void *arg acl_unused)
{
	int   operation = ACL_FLOCK_OP_SHARED;
	const char *filename;
	ACL_FILE_HANDLE   fd;

	AUT_SET_STR(test_line, "file", filename);
	fd = __mylock_try_open(filename);
	return (__mylock_op(fd, operation, NULL, 0));
}

