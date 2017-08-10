#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#endif  /* ACL_PREPARE_COMPILE */

#ifdef ACL_WINDOWS
#include "stdlib/acl_stdlib.h"
#include "net/acl_net.h"
#include <windows.h>
#include "proctl_internal.h"

void get_lock_file(char *buf, size_t size)
{
	char  full_path[MAX_PATH], driver[MAX_PATH], dir_path[MAX_PATH];
	char  file_name[MAX_PATH], ext_name[MAX_PATH];

	GetModuleFileName(NULL, full_path, MAX_PATH);
	_splitpath(full_path, driver, dir_path, file_name, ext_name);
	snprintf(buf, size, "%s%s%s.lock", driver, dir_path, file_name);
}

void get_exec_path(char *buf, size_t size)
{
	char  full_path[MAX_PATH], driver[MAX_PATH], dir_path[MAX_PATH];
	char  file_name[MAX_PATH], ext_name[MAX_PATH], *ptr;

	GetModuleFileName(NULL, full_path, MAX_PATH);
	_splitpath(full_path, driver, dir_path, file_name, ext_name);
	snprintf(buf, size, "%s%s", driver, dir_path);
	ptr = buf + strlen(buf) - 1;
	while (ptr >= buf && (*ptr == '\\' || *ptr == '/')) {
		*ptr-- = 0;
	}
}

void get_lock_file2(const char *filepath, char *buf, size_t size)
{
	char *ptr;

	snprintf(buf, size, "%s", filepath);

	ptr = strrchr(buf, '.');
	if (ptr)
		*ptr = 0;
	else
		ptr = buf + strlen(buf);

	size = buf + size - ptr;
	snprintf(ptr, size, ".lock");
}

int get_addr_from_file(const char *filepath, char *buf, size_t size)
{
	const char *myname = "get_addr_from_file";
	ACL_VSTREAM *fp;
	int   n;

	fp = acl_vstream_fopen(filepath, O_RDONLY, 0600, 1024);
	if (fp == NULL) {
		acl_msg_error("%s(%d): fopen file(%s) error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return -1;
	}

	n = acl_vstream_gets_nonl(fp, buf, size);
	acl_vstream_close(fp);

	if (n == ACL_VSTREAM_EOF)
		acl_msg_error("%s(%d): gets from file(%s) error(%s)",
			myname, __LINE__, filepath, acl_last_serror());

	return 0;
}

ACL_VSTREAM *local_listen()
{
	const char *myname = "local_listen";
	char  lock_file[MAX_PATH];
	ACL_VSTREAM *sstream, *fp;
	ACL_FILE_HANDLE handle;

	get_lock_file(lock_file, sizeof(lock_file));

	fp = acl_vstream_fopen(lock_file, O_RDWR | O_CREAT, 0600, 1024);
	if (fp == NULL)
		acl_msg_fatal("%s(%d): open file(%s) error(%s)",
			myname, __LINE__, lock_file, acl_last_serror());

	handle = ACL_VSTREAM_FILE(fp);
	if (acl_myflock(handle, 0, ACL_FLOCK_OP_EXCLUSIVE
		| ACL_FLOCK_OP_NOWAIT) == -1)
	{
		acl_msg_error("%s(%d): lock file(%s) error(%s)",
			myname, __LINE__, lock_file, acl_last_serror());
		return NULL;
	}

	sstream = acl_vstream_listen_ex("127.0.0.1:0", 128, 0, 1024, 0);
	if (sstream == NULL)
		acl_msg_fatal("%s(%d): listen error(%s)",
			myname, __LINE__, acl_last_serror());

	if (acl_file_ftruncate(fp, 0) < 0)
		acl_msg_fatal("%s(%d): truncate file(%s) error(%s)",
			myname, __LINE__, lock_file, acl_last_serror());
	if (acl_vstream_fseek(fp, 0, SEEK_SET) < 0)
		acl_msg_fatal("%s(%d): fseek file(%s) error(%s)",
			myname, __LINE__, lock_file, acl_last_serror());

	if (acl_vstream_fprintf(fp, "%s\r\n", ACL_VSTREAM_LOCAL(sstream))
		== ACL_VSTREAM_EOF)
	{
		acl_msg_fatal("%s(%d): fprintf to file(%s) error(%s)",
			myname, __LINE__, lock_file, acl_last_serror());
	}

	/* XXX: 只能采用先解排它锁，再加共享锁，微软比较弱!!! */

	if (acl_myflock(handle, 0, ACL_FLOCK_OP_NONE) == -1)
		acl_msg_fatal("%s(%d): unlock file(%s) error(%s)",
			myname, __LINE__, lock_file, acl_last_serror());
	if (acl_myflock(handle, 0, ACL_FLOCK_OP_SHARED
		| ACL_FLOCK_OP_NOWAIT) == -1)
	{
		acl_msg_fatal("%s(%d): lock file(%s) error(%s)",
			myname, __LINE__, lock_file, acl_last_serror());
	}
	return sstream;
}

#endif /* ACL_WINDOWS */
