#include "lib_acl.h"
#include <assert.h>
#include "pipeio.h"

#ifdef	ACL_UNIX

static int __max = 1000;
static ACL_VSTREAM *__sstream_reader, *__sstream_writer;
static ACL_VSTREAM *__cstream_reader, *__cstream_writer;
static int __finish = 0;

typedef struct {
	char buf[1024];
} DATA;

typedef struct {
	int  cmd;
#define	CMD_NOOP	0
#define	CMD_STOP	1
	DATA *data;
} IO_MSG;

static int __server_fd[2], __client_fd[2];

static void pipe_init(void)
{
	if (acl_duplex_pipe(__server_fd) < 0) {
		printf("acl_duplex_pipe error(%s)\r\n", acl_last_serror());
		exit (1);
	}
	if (acl_duplex_pipe(__client_fd) < 0) {
		printf("acl_duplex_pipe error(%s)\r\n", acl_last_serror());
		exit (1);
	}
	__sstream_reader = acl_vstream_fdopen(__server_fd[0], O_RDONLY, 1024, 10, ACL_VSTREAM_TYPE_SOCK);
	__sstream_writer = acl_vstream_fdopen(__client_fd[1], O_WRONLY, 1024, 10, ACL_VSTREAM_TYPE_SOCK);

	__cstream_reader = acl_vstream_fdopen(__client_fd[0], O_RDONLY, 1024, 10, ACL_VSTREAM_TYPE_SOCK);
	__cstream_writer = acl_vstream_fdopen(__server_fd[1], O_WRONLY, 1024, 10, ACL_VSTREAM_TYPE_SOCK);
}

static void pipe_end(void)
{
	acl_vstream_close(__sstream_reader);
	acl_vstream_close(__sstream_writer);

	acl_vstream_close(__cstream_reader);
	acl_vstream_close(__cstream_writer);
}

static void *client_thread(void *arg acl_unused)
{
	int   i;
	IO_MSG msg;
	DATA *data;
	time_t begin, end;

	data = (DATA *) acl_mymalloc(sizeof(DATA));
	msg.cmd = CMD_NOOP;
	msg.data = data;

	(void) time(&begin);

	for (i = 0; i < __max; i++) {
		if (acl_vstream_writen(__cstream_writer, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF) {
			printf("write to server error(%s)\r\n", acl_last_serror());
			break;
		}
		if (acl_vstream_readn(__cstream_reader, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF) {
			printf("read from server error(%s)\r\n", acl_last_serror());
			break;
		}
	}

	(void) time(&end);
	__finish = 1;
	sleep(1);
	printf("max loop: %d, time=%ld\r\n", __max, (long int) (end - begin));

	printf("send stop msg to server thread\r\n");
	msg.cmd = CMD_STOP;
	if (acl_vstream_writen(__cstream_writer, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF) {
		printf("write to server error(%s)\r\n", acl_last_serror());
	} else {
		printf("wait server thread respond\r\n");
		if (acl_vstream_readn(__cstream_reader, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF)
			printf("read server's respond error(%s)\r\n", acl_last_serror());
	}

	acl_myfree(data);
	printf("OK, client exit now\r\n");
	return (NULL);
}

static void *server_thread(void *arg acl_unused)
{
	IO_MSG msg;

	while (1) {
		if (acl_vstream_readn(__sstream_reader, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF) {
			printf("read from client error(%s)\r\n", acl_last_serror());
			break;
		}
		if (acl_vstream_writen(__sstream_writer, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF) {
			printf("write to client error(%s)\r\n", acl_last_serror());
			break;
		}
		if (msg.cmd == CMD_STOP) {
			break;
		}
	}

	sleep(1);
	printf("server thread exit now\r\n");
	return (NULL);
}

static void *waiter_thread(void *arg acl_unused)
{
	acl_vstream_printf("pipeio running ...\r\n");
	while (1) {
		if (__finish == 1)
			break;
		acl_vstream_printf(".");
		sleep(1);
	}

	acl_vstream_printf("\r\n");
	return (NULL);
}

void pipeio_run(int max)
{
	acl_pthread_t id_client, id_server, id_waiter;
	acl_pthread_attr_t attr;
	void *ptr;

	__max = max > 0 ? max : 1000;
	printf("max loop: %d\r\n", __max);

	pipe_init();

	acl_pthread_attr_init(&attr);
	acl_pthread_create(&id_server, &attr, server_thread, NULL);
	acl_pthread_create(&id_client, &attr, client_thread, NULL);
	acl_pthread_create(&id_waiter, &attr, waiter_thread, NULL);
	acl_pthread_join(id_server, &ptr);
	acl_pthread_join(id_client, &ptr);
	acl_pthread_join(id_server, &ptr);
	acl_pthread_attr_destroy(&attr);

	pipe_end();
}
#else
void pipeio_run(int max)
{
	const char *myname = "pipeio_run";

	printf("%s: not support!\r\n", myname);
}
#endif
