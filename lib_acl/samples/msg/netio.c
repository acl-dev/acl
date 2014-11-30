#include "lib_acl.h"
#include <assert.h>
#include "netio.h"

static int __max = 1000;
static char __server_addr[64];
static ACL_VSTREAM *__sstream;
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

static void server_listen(void)
{
	char  addr[] = "127.0.0.1:0";

	__sstream = acl_vstream_listen(addr, 64);
	assert(__sstream);
	snprintf(__server_addr, sizeof(__server_addr), "%s", ACL_VSTREAM_LOCAL(__sstream));
}

static void server_listen_stop(void)
{
	if (__sstream)
		acl_vstream_close(__sstream);
}

static void *client_thread(void *arg acl_unused)
{
	ACL_VSTREAM *client;
	int   i;
	IO_MSG msg;
	DATA *data;
	time_t begin, end;

	client = acl_vstream_connect(__server_addr, ACL_BLOCKING, 10, 10, 1024);
	assert(client);

	data = (DATA *) acl_mymalloc(sizeof(DATA));
	msg.cmd = CMD_NOOP;
	msg.data = data;

	(void) time(&begin);

	for (i = 0; i < __max; i++) {
		if (acl_vstream_writen(client, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF) {
			printf("write to server error(%s)\r\n", acl_last_serror());
			break;
		}
		if (acl_vstream_readn(client, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF) {
			printf("read from server error(%s)\r\n", acl_last_serror());
			break;
		}
	}

	(void) time(&end);
	__finish = 1;
	sleep(1);
	printf("max loop: %d, time=%ld\r\n", __max, (long int)(end - begin));

	printf("send stop msg to server thread\r\n");
	msg.cmd = CMD_STOP;
	if (acl_vstream_writen(client, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF) {
		printf("write to server error(%s)\r\n", acl_last_serror());
	} else {
		printf("wait server thread respond\r\n");
		if (acl_vstream_readn(client, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF)
			printf("read server's respond error(%s)\r\n", acl_last_serror());
	}

	acl_myfree(data);
	acl_vstream_close(client);
	printf("OK, client exit now\r\n");
	return (NULL);
}

static void *server_thread(void *arg acl_unused)
{
	ACL_VSTREAM *client;
	IO_MSG msg;

	client = acl_vstream_accept(__sstream, NULL, 0);
	assert(client);

	server_listen_stop();

	while (1) {
		if (acl_vstream_readn(client, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF) {
			printf("read from client error(%s)\r\n", acl_last_serror());
			break;
		}
		if (acl_vstream_writen(client, &msg, sizeof(IO_MSG)) == ACL_VSTREAM_EOF) {
			printf("write to client error(%s)\r\n", acl_last_serror());
			break;
		}
		if (msg.cmd == CMD_STOP) {
			break;
		}
	}

	acl_vstream_close(client);
	sleep(1);
	printf("server thread exit now\r\n");
	return (NULL);
}

static void *waiter_thread(void *arg acl_unused)
{
	acl_vstream_printf("netio running ...\r\n");
	while (1) {
		if (__finish == 1)
			break;
		acl_vstream_printf(".");
		sleep(1);
	}

	acl_vstream_printf("\r\n");
	return (NULL);
}

void netio_run(int max)
{
	acl_pthread_t id_client, id_server, id_waiter;
	acl_pthread_attr_t attr;
	void *ptr;

	__max = max > 0 ? max : 1000;
	printf("max loop: %d\r\n", __max);

	server_listen();

	acl_pthread_attr_init(&attr);
	acl_pthread_create(&id_server, &attr, server_thread, NULL);
	acl_pthread_create(&id_client, &attr, client_thread, NULL);
	acl_pthread_create(&id_waiter, &attr, waiter_thread, NULL);
	acl_pthread_join(id_client, &ptr);
	acl_pthread_join(id_waiter, &ptr);
	acl_pthread_join(id_server, &ptr);
	acl_pthread_attr_destroy(&attr);
}
