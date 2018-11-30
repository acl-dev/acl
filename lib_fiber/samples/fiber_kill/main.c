#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"

static ACL_FIBER *__fiber_wait1  = NULL;
static ACL_FIBER *__fiber_wait2  = NULL;
static ACL_FIBER *__fiber_sleep  = NULL;
static ACL_FIBER *__fiber_sleep2 = NULL;
static ACL_FIBER *__fiber_server = NULL;
static ACL_FIBER *__fiber_lock1  = NULL;
static ACL_FIBER *__fiber_lock2  = NULL;

static void fiber_wait(ACL_FIBER *fiber, void *ctx)
{
	ACL_FIBER_SEM *sem = (ACL_FIBER_SEM *) ctx;
	int left;

	printf("wait fiber-%d: begin to sem_wait\r\n", acl_fiber_self());
	left = acl_fiber_sem_wait(sem);
	printf("wait fiber-%d: sem_wait ok, left: %d\r\n",
		acl_fiber_self(), left);
	if (acl_fiber_killed(fiber))
		printf("wait fiber-%d: was killed\r\n", acl_fiber_id(fiber));
}

static void fiber_sleep(ACL_FIBER *fiber, void *ctx acl_unused)
{
	while (1) {
		printf("sleep fiber-%d: begin sleep\r\n", acl_fiber_self());
		sleep(2);
		printf("sleep fiber-%d: wakeup\r\n", acl_fiber_self());
		if (acl_fiber_killed(fiber)) {
			printf("sleep fiber-%d: killed\r\n",
				acl_fiber_id(fiber));
			break;
		}
	}
}

static void fiber_sleep2(ACL_FIBER *fiber, void *ctx acl_unused)
{
	while (1)
	{
		printf("sleep2 fiber-%d: %p sleep\r\n",
			acl_fiber_self(), fiber);
		sleep(1);
		printf("sleep2 fiber-%d: wakeup\r\n", acl_fiber_self());
		if (acl_fiber_killed(fiber)) {
			printf("sleep2 fiber-%d: killed\r\n",
				acl_fiber_id(fiber));
			break;
		}
	}
}

static void fiber_client(ACL_FIBER *fiber, void *ctx)
{
	ACL_VSTREAM *conn = (ACL_VSTREAM *) ctx;
	char buf[8192];
	int  ret;

	printf("client fiber-%d: accept client, local: %s, peer: %s\r\n",
		acl_fiber_self(), ACL_VSTREAM_LOCAL(conn),
		ACL_VSTREAM_PEER(conn));

	while (1) {
		ret = acl_vstream_gets(conn, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("client fiber-%d: gets error %s\r\n",
				acl_fiber_id(fiber), acl_last_serror());
			break;
		}
		if (acl_vstream_writen(conn, buf, ret) != ret) {
			printf("client fiber-%d: write error %s\r\n",
				acl_fiber_id(fiber), acl_last_serror());
			break;
		}
	}

	acl_vstream_close(conn);
}

static void fiber_server(ACL_FIBER *fiber, void *ctx acl_unused)
{
	ACL_VSTREAM *server = acl_vstream_listen("127.0.0.1:9005", 128);

	if (server == NULL) {
		printf("server fiber-%d: listen error %s\r\n",
			acl_fiber_self(), acl_last_serror());
		exit (1);
	}
	printf("server fiber-%d: listen %s ok\r\n", acl_fiber_self(),
		ACL_VSTREAM_LOCAL(server));

	while (1) {
		ACL_VSTREAM *conn = acl_vstream_accept(server, NULL, 0);
		if (conn == NULL) {
			printf("server fiber-%d: accept error %s\r\n",
				acl_fiber_self(), acl_last_serror());
			if (acl_fiber_killed(fiber)) {
				printf("server fiber-%d: killed\r\n",
					acl_fiber_self());
				break;
			}
		}
		else
			printf("----accept one client ----\r\n");

		acl_fiber_create(fiber_client, conn, 320000);
	}

	acl_vstream_close(server);
}

static void fiber_lock(ACL_FIBER *fiber, void *ctx)
{
	ACL_FIBER_MUTEX *lock = (ACL_FIBER_MUTEX *) ctx;

	printf("lock fiber-%d: begin to lock\r\n", acl_fiber_self());
	acl_fiber_mutex_lock(lock);
	if (acl_fiber_killed(fiber))
		printf("lock fiber-%d: killed\r\n", acl_fiber_self());
	else {
		printf("lock fiber-%d: lock ok, sleep\r\n", acl_fiber_self());
		sleep(1000);
		if (acl_fiber_killed(fiber))
			printf("lock fiber-%d: killed\r\n", acl_fiber_self());
		else
			printf("lock fiber-%d: wakeup\r\n", acl_fiber_self());
	}
}

static void fiber_killer(ACL_FIBER *fiber, void *ctx acl_unused)
{
	acl_fiber_sleep(1);

	acl_fiber_kill(__fiber_wait1);
	acl_fiber_kill(__fiber_wait2);

	acl_fiber_sleep(1);
	acl_fiber_kill(__fiber_sleep);
	acl_fiber_kill(__fiber_sleep2);
	acl_fiber_kill(__fiber_server);
	acl_fiber_kill(__fiber_lock1);
	acl_fiber_kill(__fiber_lock2);

	printf("=====all fiber are killed, %d, %p=======\r\n",
		acl_fiber_self(), fiber);

	acl_fiber_schedule_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;
	ACL_FIBER_SEM *sem;
	ACL_FIBER_MUTEX *lock;

	acl_msg_stdout_enable(1);

	while ((ch = getopt(argc, argv, "hn:c:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}

	sem = acl_fiber_sem_create(0);
	lock = acl_fiber_mutex_create();

	__fiber_wait1  = acl_fiber_create(fiber_wait, sem, 320000);
	__fiber_wait2  = acl_fiber_create(fiber_wait, sem, 320000);
	__fiber_sleep  = acl_fiber_create(fiber_sleep, sem, 320000);
	__fiber_server = acl_fiber_create(fiber_server, NULL, 320000);
	__fiber_lock1  = acl_fiber_create(fiber_lock, lock, 320000);
	__fiber_lock2  = acl_fiber_create(fiber_lock, lock, 320000);

	__fiber_sleep2 = acl_fiber_create(fiber_sleep2, NULL, 320000);
	acl_fiber_create(fiber_killer, NULL, 320000);

	acl_fiber_schedule();
	acl_fiber_sem_free(sem);
	acl_fiber_mutex_free(lock);

	return 0;
}
