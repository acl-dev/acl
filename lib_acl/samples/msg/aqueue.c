#include "lib_acl.h"
#include <time.h>
#include "aqueue.h"

static int   __max = 1000;
static ACL_AQUEUE *__client_queue, *__server_queue;
static int __finish = 0;

typedef struct {
	int  cmd;
#define	CMD_NOOP	0
#define	CMD_STOP	1
	char buf[1024];
} DATA;

static void *client_thread(void *arg acl_unused)
{
	int   i;
	time_t begin, end;
	DATA *data, *data_saved;

	data = (DATA*) acl_mymalloc(1024);
	data->cmd = CMD_NOOP;
	data_saved = data;

	(void) time(&begin);
	for (i = 0; i < __max; i++) {

		if (acl_aqueue_push(__server_queue, data) < 0) {
			printf("add to server queue error(%s)\r\n", acl_last_serror());
			break;
		}
		data = acl_aqueue_pop(__client_queue);
		if (data == NULL) {
			printf("pop from client queue error(%s)\r\n", acl_last_serror());
			break;
		}
	}

	(void) time(&end);
	__finish = 1;
	sleep(1);
	printf("max loop: %d, time=%ld\r\n", __max, (long int)(end - begin));

	printf("send stop msg to server thread\r\n");
	data->cmd = CMD_STOP;
	if (acl_aqueue_push(__server_queue, data) < 0)
		printf("push stop msg to server queue error(%s)\r\n", acl_last_serror());
	else {
		printf("wait server thread respond\r\n");
		data = (DATA *) acl_aqueue_pop(__client_queue);
		if (data == NULL)
			printf("pop from client queue error(%s)\r\n", acl_last_serror());
	}

	acl_myfree(data_saved);
	printf("OK, client exit now\r\n");
	return (NULL);
}

static void *server_thread(void *arg acl_unused)
{
	int   cmd;
	DATA *data;

	while (1) {
		data = (DATA *) acl_aqueue_pop(__server_queue);
		if (data == NULL) {
			printf("pop from server queue error(%s)\r\n", acl_last_serror());
			break;
		}

		cmd = data->cmd;
		if (acl_aqueue_push(__client_queue, data) < 0) {
			printf("push to client queue error(%s)\r\n", acl_last_serror());
			break;
		}
		if (cmd == CMD_STOP) {
			break;
		}
	}

	sleep(1);
	printf("server thread exit now\r\n");
	return (NULL);
}

static void *waiter_thread(void *arg acl_unused)
{
	acl_vstream_printf("aqueue running ...\r\n");
	while (1) {
		if (__finish == 1)
			break;
		acl_vstream_printf(".");
		sleep(1);
	}

	acl_vstream_printf("\r\n");
	return (NULL);
}

void aqueue_run(int max)
{
	acl_pthread_t id_client, id_server, id_waiter;
	acl_pthread_attr_t attr;
	void *ptr;

	__max = max > 0 ? max : 1000;

	printf("max loop: %d\r\n", __max);
	__client_queue = acl_aqueue_new();
	__server_queue = acl_aqueue_new();

	acl_pthread_attr_init(&attr);
	acl_pthread_create(&id_server, &attr, server_thread, NULL);
	acl_pthread_create(&id_client, &attr, client_thread, NULL);
	acl_pthread_create(&id_waiter, &attr, waiter_thread, NULL);
	acl_pthread_join(id_server, &ptr);
	acl_pthread_join(id_client, &ptr);
	acl_pthread_join(id_server, &ptr);
	acl_aqueue_free(__client_queue, NULL);
	acl_aqueue_free(__server_queue, NULL);
	acl_pthread_attr_destroy(&attr);
}
