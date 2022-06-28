#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef	WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

static void child(int argc acl_unused, char *argv[] acl_unused)
{
	const char *myname = "child";
	const char *env_list;
	ACL_VSTRING *vbuf = acl_vstring_alloc(256);
	char  buf[1024];
	const char *ptr;
	int   ret, i;

	acl_lib_init();

	env_list = acl_getenv_list();
	acl_vstring_sprintf(vbuf, "env_list(%s)", env_list);

	acl_vstring_strcat(vbuf, "), ");
	ptr = acl_getenv("name1");
	if (ptr == NULL)
		ptr = "null";
	acl_vstring_sprintf_append(vbuf, "name1=%s", ptr);

	ptr = acl_getenv("name2");
	if (ptr == NULL)
		ptr = "null";
	acl_vstring_sprintf_append(vbuf, ", name2=%s", ptr);

	ptr = acl_getenv("name3");
	if (ptr == NULL)
		ptr = "null";
	acl_vstring_sprintf_append(vbuf, ", name3=%s", ptr);

	printf("%s: %s\r\n", myname, acl_vstring_str(vbuf));
	fflush(stdout);

	i = 0;
	while (1) {
		ret = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF) {
			printf("%s(%d): gets from parent error(%s)\r\n",
				myname, getpid(), acl_last_serror());
			fflush(stdout);
			break;
		}
		ret = acl_vstream_fprintf(ACL_VSTREAM_OUT, "%s(%d): child ok\r\n",
				myname, getpid());
		if (ret == ACL_VSTREAM_EOF) {
			printf("%s(%d): fprintf to parent error(%s)\r\n",
				myname, getpid(), acl_last_serror());
			fflush(stdout);
			break;
		}
		if (i++ >= 5)
			break;
	}

	printf("%s(%d): child over\r\n", myname, getpid());
	fflush(stdout);
	acl_lib_end();
}

static void pipe_fd_status(ACL_FILE_HANDLE fd)
{
#ifdef ACL_MS_WINDOWS
	const char *myname = "pipe_fd_status";
	char  buf[256];
	DWORD   status, nInstances = 0, nMaxInstances = 0, nTimeout = 0;

	GetNamedPipeHandleState(fd, &status,
		&nInstances, NULL, NULL, buf, sizeof(buf));
	printf("%s: status: %d, %d, %d, nInstances: %d, nMaxInstances: %d, nTimeout: %d, buf: %s\r\n",
		myname, status, PIPE_WAIT, PIPE_NOWAIT, nInstances, nMaxInstances, nTimeout, buf);
#else
	(void) fd;
#endif
}

static void pipe_status(ACL_VSTREAM *stream)
{
	pipe_fd_status(ACL_VSTREAM_FILE(stream));
}

static void parent(int argc acl_unused, char *argv[])
{
	const char *myname = "parent";
	char  *command;
	ACL_ARGV *env = acl_argv_split("name1&value1&name2&变量值2&name3&变量值4", "&");
	ACL_VSTREAM *stream;
	ACL_ARGV *args = acl_argv_alloc(1);
	char  buf[1024];
	int   ret, i;

	acl_lib_init();
	printf("%s: pid = %d\r\n", myname, getpid());
	command = argv[0];
	acl_argv_add(args, command, "-c", NULL);

	stream = acl_vstream_popen(O_RDWR,
				ACL_VSTREAM_POPEN_ARGV, args->argv,
				ACL_VSTREAM_POPEN_ENV, env->argv,
				ACL_VSTREAM_POPEN_END);
	if (stream == NULL) {
		acl_msg_error("%s(%d): popen %s error(%s)",
			myname, __LINE__, command, acl_last_serror());
		goto error_end;
	}

	pipe_status(stream);

	printf("%s: open pipe ok\r\n", myname);

	i = 0;
	while (1) {
		pipe_status(stream);
		ret =acl_vstream_fprintf(stream, ">>from server<<\r\n");
		if (ret == ACL_VSTREAM_EOF) {
			printf("%s: fprintf to child error(%s)\r\n",
				myname, acl_last_serror());
			break;
		} else
			printf("%s: fprintf to child ok\r\n", myname);

		printf("%s: begin wait client msg\n", myname);
		ret = acl_vstream_gets_nonl(stream, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF) {
			printf("%s: gets from child error(%s)\r\n",
				myname, acl_last_serror());
			break;
		}
		buf[ret] = 0;
		printf("%s: gets from child ok(%s), i=%d\r\n", myname, buf, i++);
	}

	acl_vstream_pclose(stream);

error_end:

	acl_argv_free(env);
	acl_argv_free(args);
	acl_lib_end();
}

static void usage(const char *procname)
{
	printf("(%d) usage: %s -h [help] -c [run as child] -p [run as parent]\r\n",
		getpid(), procname);
#ifdef ACL_MS_WINDOWS
	printf("enter any key to exit...\r\n");
	getchar();
#endif
}


int main(int argc, char *argv[])
{
	char  ch;

	while((ch = getopt(argc, argv, "hpc")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'c':
			child(argc, argv);
			break;
		case 'p':
			parent(argc, argv);
			break;
		default:
			usage(argv[0]);
			return (0);
		}
	}

	printf("pid(%d), exit now\n", getpid());
	return (0);
}
