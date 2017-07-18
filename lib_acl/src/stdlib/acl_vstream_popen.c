#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_msg.h"
#include <stdarg.h>
#include <errno.h>

#include "stdlib/acl_argv.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/unix/acl_set_ugid.h"
#include "stdlib/acl_env.h"
#include "stdlib/acl_exec_command.h"
#include "stdlib/acl_vstream_popen.h"

#ifdef	ACL_UNIX
#include <unistd.h>
#include <sys/wait.h>
#endif

#endif

/* Application-specific. */

#ifdef	ACL_UNIX
typedef int (*ACL_VSTREAM_WAITPID_FN) (pid_t, ACL_WAIT_STATUS_T *, int);
#endif

typedef struct ACL_VSTREAM_POPEN_ARGS {
	char  **argv;
	char   *command;
	int     privileged;
	char  **env;
	char  **export;
	char   *shell;
#ifdef	ACL_UNIX
	uid_t   uid;
	gid_t   gid;
	ACL_VSTREAM_WAITPID_FN waitpid_fn;
#endif
} ACL_VSTREAM_POPEN_ARGS;

/* vstream_parse_args - get arguments from variadic list */

static void vstream_parse_args(ACL_VSTREAM_POPEN_ARGS *args, va_list ap)
{
	const char *myname = "vstream_parse_args";
	int     key;

	/*
	 * First, set the default values (on all non-zero entries)
	 */
	args->argv = 0;
	args->command = 0;
	args->privileged = 0;
	args->env = 0;
	args->export = 0;
	args->shell = 0;
#ifdef ACL_UNIX
	args->uid = 0;
	args->gid = 0;
	args->waitpid_fn = 0;
#endif

	/*
	 * Then, override the defaults with user-supplied inputs.
	 */
	while ((key = va_arg(ap, int)) != ACL_VSTREAM_POPEN_END) {
		switch (key) {
		case ACL_VSTREAM_POPEN_ARGV:
			if (args->command != 0)
				acl_msg_panic("%s: got ACL_VSTREAM_POPEN_ARGV"
					" and ACL_VSTREAM_POPEN_COMMAND", myname);
			args->argv = va_arg(ap, char **);
			break;
		case ACL_VSTREAM_POPEN_COMMAND:
			if (args->argv != 0)
				acl_msg_panic("%s: got ACL_VSTREAM_POPEN_ARGV"
					" and ACL_VSTREAM_POPEN_COMMAND", myname);
			args->command = va_arg(ap, char *);
			break;
		case ACL_VSTREAM_POPEN_ENV:
			args->env = va_arg(ap, char **);
			break;
		case ACL_VSTREAM_POPEN_EXPORT:
			args->export = va_arg(ap, char **);
			break;
		case ACL_VSTREAM_POPEN_SHELL:
			args->shell = va_arg(ap, char *);
			break;
#ifdef	ACL_UNIX
		case ACL_VSTREAM_POPEN_UID:
			args->privileged = 1;
#ifdef MINGW
			args->uid = (uid_t) va_arg(ap, int);
#else
			args->uid = va_arg(ap, uid_t);
#endif
			break;
		case ACL_VSTREAM_POPEN_GID:
			args->privileged = 1;
#ifdef MINGW
			args->gid = (gid_t) va_arg(ap, int);
#else
			args->gid = va_arg(ap, gid_t);
#endif
			break;
		case ACL_VSTREAM_POPEN_WAITPID_FN:
			args->waitpid_fn = va_arg(ap, ACL_VSTREAM_WAITPID_FN);
			break;
#endif
		default:
			acl_msg_panic("%s: unknown key: %d", myname, key);
		}
	}

	if (args->command == 0 && args->argv == 0)
		acl_msg_panic("%s: missing ACL_VSTREAM_POPEN_ARGV"
			" or ACL_VSTREAM_POPEN_COMMAND", myname);
#ifdef	ACL_UNIX
	if (args->privileged != 0 && args->uid == 0)
		acl_msg_panic("%s: privileged uid", myname);
	if (args->privileged != 0 && args->gid == 0)
		acl_msg_panic("%s: privileged gid", myname);
#endif
}

#ifdef	ACL_UNIX

/* acl_vstream_popen - open fp to child process */

ACL_VSTREAM *acl_vstream_popen(int flags,...)
{
	const char *myname = "acl_vstream_popen";
	ACL_VSTREAM_POPEN_ARGS args;
	va_list ap;
	ACL_VSTREAM *fp;
	int     sockfd[2];
	int     pid;
	int     fd;
	ACL_ARGV   *argv;
	char  **cpp;

	va_start(ap, flags);
	vstream_parse_args(&args, ap);
	va_end(ap);

	if (args.command == 0)
		args.command = args.argv[0];

	if (acl_duplex_pipe(sockfd) < 0)
		return 0;

	switch (pid = fork()) {
	case -1:				/* error */
		(void) close(sockfd[0]);
		(void) close(sockfd[1]);
		return 0;
	case 0:					/* child */
		if (close(sockfd[1]))
			acl_msg_warn("close: %s", acl_last_serror());
		for (fd = 0; fd < 2; fd++)
			if (sockfd[0] != fd && DUP2(sockfd[0], fd) < 0)
				acl_msg_fatal("dup2: %s", acl_last_serror());
		if (sockfd[0] >= 2 && close(sockfd[0]))
			acl_msg_warn("close: %s", acl_last_serror());

		/*
		 * Don't try to become someone else unless the user specified it.
		 */
		if (args.privileged)
			acl_set_ugid(args.uid, args.gid);

		/*
		 * Environment plumbing. Always reset the command search path. XXX
		 * That should probably be done by clean_env().
		 */
		if (args.export)
			acl_clean_env(args.export);
		if (setenv("PATH", ACL_PATH_DEFPATH, 1))
			acl_msg_fatal("%s: setenv: %s", myname, acl_last_serror());
		if (args.env)
			for (cpp = args.env; *cpp; cpp += 2)
				if (setenv(cpp[0], cpp[1], 1))
					acl_msg_fatal("setenv: %s", acl_last_serror());

		/*
		 * Process plumbing. If possible, avoid running a shell.
		 */
		acl_msg_close();
		if (args.argv) {
			execvp(args.argv[0], args.argv);
			acl_msg_fatal("%s: execvp %s: %s",
				myname, args.argv[0], acl_last_serror());
		} else if (args.shell && *args.shell) {
			argv = acl_argv_split(args.shell, " \t\r\n");
			acl_argv_add(argv, args.command, (char *) 0);
			acl_argv_terminate(argv);
			execvp(argv->argv[0], argv->argv);
			acl_msg_fatal("%s: execvp %s: %s",
				myname, argv->argv[0], acl_last_serror());
		} else {
			acl_exec_command(args.command);
		}
		/* NOTREACHED */
		return NULL;
	default:					/* parent */
		if (close(sockfd[0]))
			acl_msg_warn("close: %s", acl_last_serror());
		fp = acl_vstream_fdopen(sockfd[1], flags,
				4096, 0, ACL_VSTREAM_TYPE_FILE);
		/*
		 * fp->waitpid_fn = args.waitpid_fn;
		 */
		fp->pid = pid;
		return fp;
	}
}

/* acl_vstream_pclose - close fp to child process */

int acl_vstream_pclose(ACL_VSTREAM *fp)
{
	pid_t   saved_pid = fp->pid;
	/*
	 * ACL_VSTREAM_WAITPID_FN saved_waitpid_fn = fp->waitpid_fn;
	 */
	ACL_VSTREAM_WAITPID_FN saved_waitpid_fn = 0;
	pid_t   pid;
	ACL_WAIT_STATUS_T wait_status;

	/*
	 * Close the pipe. Don't trigger an alarm in vstream_fclose().
	 */
	if (saved_pid == 0)
		acl_msg_panic("vstream_pclose: fp has no process");
	fp->pid = 0;
	acl_vstream_fclose(fp);

	/*
	 * Reap the child exit status.
	 */
	do {
		if (saved_waitpid_fn != 0)
			pid = saved_waitpid_fn(saved_pid, &wait_status, 0);
		else
			pid = waitpid(saved_pid, &wait_status, 0);
	} while (pid == -1 && errno == EINTR);
	return pid == -1 ? -1 :
		WIFSIGNALED(wait_status) ? WTERMSIG(wait_status) :
		WEXITSTATUS(wait_status);
}

#elif defined(ACL_WINDOWS)

#include <process.h>
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_sys_patch.h"

/* acl_vstream_popen - open fp to child process */

ACL_VSTREAM *acl_vstream_popen(int flags,...)
{
	const char *myname = "acl_vstream_popen";
	ACL_VSTREAM_POPEN_ARGS args;
	va_list ap;
	ACL_VSTREAM *fp;
	ACL_FILE_HANDLE  fds[2];
	HANDLE prochnd, hInOut;
	STARTUPINFOA sinfo;
	PROCESS_INFORMATION pinfo;
	DWORD dwCreationFlags = 0;
	ACL_VSTRING *envbuf, *cmdline = acl_vstring_alloc(256);
	char **cpp;

	va_start(ap, flags);
	vstream_parse_args(&args, ap);
	va_end(ap);

	if (args.command == 0) {
		int   i;

		args.command = args.argv[0];

		/* 组建启动进程命令行参数表 */
		/* 为了避免参数传递时可能因其中间含有空格而被分隔成
		 * 多个参数，所以需要在参数两边加上引号
		 */
		for (i = 0; args.argv[i] != NULL; i++) {
			acl_vstring_strcat(cmdline, "\"");
			acl_vstring_strcat(cmdline, args.argv[i]);
			acl_vstring_strcat(cmdline, "\" ");
		}
	} else {
		acl_vstring_strcpy(cmdline, "\"");
		acl_vstring_strcat(cmdline, args.command);
		acl_vstring_strcat(cmdline, "\"");
	}

	if (acl_duplex_pipe(fds) < 0) {
		acl_vstring_free(cmdline);
		return NULL;
	}

	prochnd = GetCurrentProcess();
	if (!DuplicateHandle(prochnd, fds[1], prochnd, &hInOut,
		0L, TRUE, DUPLICATE_SAME_ACCESS))
	{
		acl_msg_error("%s(%d): DuplicateHandle error(%s)",
			myname, __LINE__, acl_last_serror());
		acl_file_close(fds);
		acl_vstring_free(cmdline);
		return NULL;
	}

	if (args.env) {
		envbuf = acl_vstring_alloc(256);
		for (cpp = args.env; *cpp; cpp += 2) {
			acl_vstring_memcat(envbuf, cpp[0], strlen(cpp[0]));
			ACL_VSTRING_ADDCH(envbuf, '=');
			acl_vstring_memcat(envbuf, cpp[1], strlen(cpp[1]));
			ACL_VSTRING_ADDCH(envbuf, '\0');
		}
		if (ACL_VSTRING_LEN(envbuf) == 0)
			ACL_VSTRING_ADDCH(envbuf, '\0');
		ACL_VSTRING_ADDCH(envbuf, '\0');
	} else {
		envbuf = NULL;
	}

	memset(&sinfo, 0, sizeof(sinfo));
	sinfo.cb = sizeof(sinfo);
	/* sinfo.dwFlags = STARTF_USESHOWWINDOW;*/
	sinfo.dwFlags |= STARTF_USESTDHANDLES;
	/* sinfo.wShowWindow = SW_HIDE; */
	sinfo.hStdInput = hInOut;
	sinfo.hStdOutput = hInOut;
	sinfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	if (!CreateProcessA(args.command, acl_vstring_str(cmdline), /* Command line */
		NULL, NULL,  /* Proc & thread security attributes */
		TRUE,  /* Inherit handles */
		dwCreationFlags,  /* Creation flags */
		envbuf ? acl_vstring_str(envbuf) : NULL,  /* Environment block */
		NULL,  /* Current directory name */
		&sinfo, &pinfo))
	{
		acl_msg_error("%s: CreateProcess(%s) error(%s)",
			myname, args.command, acl_last_serror());
		CloseHandle(pinfo.hThread);
		acl_file_close(fds);
		if (envbuf)
			acl_vstring_free(envbuf);
		acl_vstring_free(cmdline);
		return NULL;
	}

	acl_file_close(fds[1]);
	acl_file_close(hInOut);
	CloseHandle(pinfo.hThread);
	fp = acl_vstream_fhopen(fds[0], flags);
	fp->pid = pinfo.dwProcessId;
	fp->hproc = pinfo.hProcess;

	if (envbuf)
		acl_vstring_free(envbuf);
	acl_vstring_free(cmdline);
	return fp;
}

/* wail_child -- wait the child exit status */

static void wait_child(ACL_VSTREAM *fp)
{
	const char *myname = "wait_child";
	DWORD   wait_status, ntime = INFINITE;

	if (fp->hproc == INVALID_HANDLE_VALUE)
		return;

	wait_status = WaitForSingleObject(fp->hproc, ntime);
	if (wait_status == WAIT_OBJECT_0) {
		if (GetExitCodeProcess(fp->hproc, &wait_status)) {
			if (wait_status != 0)
				acl_msg_warn("%s(%d): child exit code(%d)",
					myname, __LINE__, wait_status);
		}
	} else if (wait_status == WAIT_TIMEOUT) {
		acl_msg_warn("%s(%d): wait child timeout", myname, __LINE__);
	} else {
		acl_msg_warn("%s(%d): wait child error(%s)",
			myname, __LINE__, acl_last_serror());
	}

	CloseHandle(fp->hproc);
	fp->hproc = INVALID_HANDLE_VALUE;
}

/* acl_vstream_pclose - close fp to child process */

int acl_vstream_pclose(ACL_VSTREAM *fp)
{
	const char *myname = "acl_vstream_pclose";

	/*
	 * Reap the monitor_child_thread exit status.
	 */
	wait_child(fp);
	acl_vstream_fclose(fp);
	return -1;
}

#endif /* ACL_WINDOWS */
