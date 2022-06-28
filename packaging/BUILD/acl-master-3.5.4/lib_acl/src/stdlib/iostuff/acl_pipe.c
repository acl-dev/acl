#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_iostuff.h"

#endif

#ifdef ACL_WINDOWS

#include "thread/acl_pthread.h"
#include <process.h>

int acl_pipe(ACL_FILE_HANDLE fds[2])
{
	const char *myname = "acl_pipe";
	DWORD dwPipeMode;
	DWORD dwOpenMode;
	char name[250];
	SECURITY_ATTRIBUTES sa;
	static unsigned long id = 0;

	InterlockedIncrement(&id);

	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	/* Create the read/write end of the pipe */

	dwOpenMode = PIPE_ACCESS_DUPLEX;
	dwPipeMode = PIPE_WAIT; /* PIPE_NOWAIT; */
	snprintf(name, sizeof(name), "\\\\.\\pipe\\acl-pipe-%u-%lu-%lu",
		_getpid(), (unsigned long) acl_pthread_self(), id);
	fds[0] = CreateNamedPipe(name,
				dwOpenMode,
				dwPipeMode,
				1,            /* nMaxInstances */
				65536,        /* nOutBufferSize */
				65536,        /* nInBufferSize */
				1,            /* nDefaultTimeOut */
				&sa);

	/* Create the read/write end of the pipe */
	dwOpenMode = FILE_ATTRIBUTE_NORMAL;
	fds[1] = CreateFile(name,
				GENERIC_WRITE | GENERIC_READ,   /* access mode */
				0,               /* share mode */
				NULL,            /* Security attributes */
				OPEN_EXISTING,   /* dwCreationDisposition */
				dwOpenMode,      /* Pipe attributes */
				NULL);           /* handle to template file */

	if (fds[1] == ACL_FILE_INVALID) {
		acl_msg_error("%s(%d): CreateFile(%s) error(%s)",
			myname, __LINE__, name, acl_last_serror());
		acl_file_close(fds[0]);
		return (-1);
	}
	return (0);
}

#elif defined(ACL_UNIX)

#include <unistd.h>

int acl_pipe(ACL_FILE_HANDLE fds[2])
{
	return (pipe(fds));
}

#endif

int acl_pipe_close(ACL_FILE_HANDLE fds[2])
{
	return ((acl_file_close(fds[0]) == 0 && acl_file_close(fds[1]) == 0) ? 0 : -1);
}
