#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

/* System library. */

#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_myflock.h"
#include "stdlib/acl_sys_patch.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include <errno.h>

#endif

#ifdef ACL_UNIX

#include <unistd.h>
#include <stdio.h>  /* printf() */

#include <string.h>

#ifdef ACL_HAS_FCNTL_LOCK
#include <fcntl.h>
#include <string.h>
#endif

#ifdef ACL_HAS_FLOCK_LOCK
#include <sys/file.h>
#endif

/* acl_myflock - lock/unlock entire open file */

int acl_myflock(ACL_FILE_HANDLE fd, int lock_style, int operation)
{
	int     status = 0;

	/*
	 * Sanity check.
	 */
	if ((operation & (ACL_FLOCK_OP_BITS)) != operation)
		acl_msg_panic("myflock: improper operation type: 0x%x", operation);

	switch (lock_style) {

	/*
	 * flock() does exactly what we need. Too bad it is not standard.
	 */
#ifdef ACL_HAS_FLOCK_LOCK
	case ACL_FLOCK_STYLE_FLOCK:
	{
		static int lock_ops[] = {
			LOCK_UN, LOCK_SH, LOCK_EX, -1,
			-1, LOCK_SH | LOCK_NB, LOCK_EX | LOCK_NB, -1
		};

		status = flock(fd, lock_ops[operation]);
		break;
	}
#endif

	/*
	 * fcntl() is standard and does more than we need, but we can handle
	 * it.
	 */
#ifdef ACL_HAS_FCNTL_LOCK
	case ACL_FLOCK_STYLE_FCNTL:
	{
		struct flock lock;
		int     request;
		static int lock_ops[] = {
			F_UNLCK, F_RDLCK, F_WRLCK
		};

		memset((char *) &lock, 0, sizeof(lock));
		lock.l_type = lock_ops[operation & ~ACL_FLOCK_OP_NOWAIT];
		request = (operation & ACL_FLOCK_OP_NOWAIT) ? F_SETLK : F_SETLKW;
		while ((status = fcntl(fd, request, &lock)) < 0
			&& request == F_SETLKW
			&& (acl_last_error() == ACL_EINTR
			    || acl_last_error() == ENOLCK
				|| acl_last_error() == EDEADLK))
			sleep(1);
		break;
	}
#endif
	default:
		acl_msg_panic("myflock: unsupported lock style: 0x%x", lock_style);
	}

	/*
	 * Return a consistent result. Some systems return EACCES when a lock is
	 * taken by someone else, and that would complicate error processing.
	 */
	if (status < 0 && (operation & ACL_FLOCK_OP_NOWAIT) != 0) {
		char  error = acl_last_error();
		if (error == ACL_EAGAIN || error == ACL_EWOULDBLOCK || error == EACCES)
			acl_set_error(ACL_EAGAIN);
	}

	return (status);
}
#endif /* ACL_UNIX */

#ifdef ACL_WINDOWS
# ifdef USE_LOCK_FILE
/* use LockFile/UnlockFile */
int acl_myflock(ACL_FILE_HANDLE fd, int lock_style acl_unused, int operation)
{
	const char *myname = "acl_myflock";
	DWORD size = 0xFFFFFFFF;
	char  ebuf[256];
	unsigned char lock_op;

	if ((operation & (ACL_FLOCK_OP_BITS)) != operation)
		acl_msg_panic("myflock: improper operation type: 0x%x", operation);

	lock_op = operation & ~ACL_FLOCK_OP_NOWAIT;

	if (lock_op == ACL_FLOCK_OP_NONE) {
		if(UnlockFile(fd, 0, 0, size, 0))
			return (0);

		acl_msg_error("%s(%d): unlock error(%s)",
			myname, __LINE__, acl_last_strerror(ebuf,sizeof(ebuf)));
		return (-1);
	} else if (lock_op == ACL_FLOCK_OP_SHARED) {
		while (1) {
			if(LockFile(fd, 0, 0, size, 0))
				return (0);

			if ((operation & ACL_FLOCK_OP_NOWAIT))
				return (-1);
			sleep(1);
		}
	} else if (lock_op == ACL_FLOCK_OP_EXCLUSIVE) {
		while (1) {
			if(LockFile(fd, 0, 0, size, 0))
				return (0);

			if ((operation & ACL_FLOCK_OP_NOWAIT))
				return (-1);
			sleep(1);
		}
	}

	acl_msg_error("%s(%d): invalid lock_op(%d)", myname, __LINE__, lock_op);
	return (-1);
}
# else
/* use LockFileEx/UnlockFileEx */
int acl_myflock(ACL_FILE_HANDLE fd, int lock_style acl_unused, int operation)
{
	const char *myname = "acl_myflock";
	DWORD size = 0xFFFFFFFF;
	char  ebuf[256];
	unsigned char lock_op;
	OVERLAPPED ovlp;
	DWORD flags;

	if ((operation & (ACL_FLOCK_OP_BITS)) != operation)
		acl_msg_panic("myflock: improper operation type: 0x%x", operation);

	memset(&ovlp, 0, sizeof(ovlp));

	ovlp.Offset = 0;
	lock_op = operation & ~ACL_FLOCK_OP_NOWAIT;
	if (lock_op == ACL_FLOCK_OP_NONE) {
		if(UnlockFileEx(fd, 0, 1, 0, &ovlp))
			return (0);
		acl_msg_error("%s(%d): unlock error(%s)",
			myname, __LINE__, acl_last_strerror(ebuf,sizeof(ebuf)));
		return (-1);
	} else if (lock_op == ACL_FLOCK_OP_SHARED) {
		while (1) {
			flags = 0;
			if ((operation & ACL_FLOCK_OP_NOWAIT))
				flags |= LOCKFILE_FAIL_IMMEDIATELY;

			if(LockFileEx(fd, flags, 0, 1, 0, &ovlp))
				return (0);
			if ((operation & ACL_FLOCK_OP_NOWAIT)) {
				acl_msg_error("%s(%d): lock error(%s)", myname, __LINE__,
					acl_last_strerror(ebuf,sizeof(ebuf)));
				return (-1);
			}
			sleep(1);
		}
	} else if (lock_op == ACL_FLOCK_OP_EXCLUSIVE) {
		while (1) {
			flags = LOCKFILE_EXCLUSIVE_LOCK;
			if ((operation & ACL_FLOCK_OP_NOWAIT))
				flags |= LOCKFILE_FAIL_IMMEDIATELY;

			if(LockFileEx(fd, flags, 0, 1, 0, &ovlp))
				return (0);

			if ((operation & ACL_FLOCK_OP_NOWAIT)) {
				acl_msg_error("%s(%d): lock error(%s)", myname, __LINE__,
					acl_last_strerror(ebuf,sizeof(ebuf)));
				return (-1);
			}
			sleep(1);
		}
	}

	acl_msg_error("%s(%d): invalid lock_op(%d)", myname, __LINE__, lock_op);
	return (-1);
}
# endif /* #if 0 */
#endif


