#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"

static ACL_FIBER *__fiber_wait1 = NULL;
static ACL_FIBER *__fiber_wait2 = NULL;
static ACL_FIBER *__fiber_sleep = NULL;
static ACL_FIBER *__fiber_sleep2 = NULL;
static ACL_FIBER *__fiber_server = NULL;

static void fiber_wait(ACL_FIBER *fiber, void *ctx)
{
	ACL_FIBER_SEM *sem = (ACL_FIBER_SEM *) ctx;
	int left;

	printf("fiber-%d begin to sem_wait\r\n", acl_fiber_self());
	left = acl_fiber_sem_wait(sem);
	printf("fiber-%d sem_wait ok, left: %d\r\n", acl_fiber_self(), left);
	if (acl_fiber_killed(fiber))
		printf("fiber-%d was killed\r\n", acl_fiber_id(fiber));
}

static void fiber_sleep(ACL_FIBER *fiber, void *ctx acl_unused)
{
	while (1) {
		printf("fiber-%d begin sleep\r\n", acl_fiber_self());
		acl_fiber_sleep(2);
		printf("fiber-%d wakeup\r\n", acl_fiber_self());
		if (acl_fiber_killed(fiber)) {
			printf("fiber-%d was killed\r\n", acl_fiber_id(fiber));
			break;
		}
	}
}

static void fiber_sleep2(ACL_FIBER *fiber, void *ctx acl_unused)
{
	while (1)
	{
		printf("-----fiber-%d, %p sleep ---\r\n",
			acl_fiber_self(), fiber);
		sleep(1);
		printf("-----fiber-%d wakup ---\r\n", acl_fiber_self());
		if (acl_fiber_killed(fiber)) {
			printf("fiber-%d was killed\r\n", acl_fiber_id(fiber));
			break;
		}
	}
}

static void fiber_client(ACL_FIBER *fiber, void *ctx)
{
	ACL_VSTREAM *conn = (ACL_VSTREAM *) ctx;
	char buf[8192];
	int  ret;

	printf("fiber-%d: accept client, local: %s, peer: %s\r\n",
		acl_fiber_self(), ACL_VSTREAM_LOCAL(conn),
		ACL_VSTREAM_PEER(conn));

	while (1) {
		ret = acl_vstream_gets(conn, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("fiber-%d: gets error %s\r\n",
				acl_fiber_id(fiber), acl_last_serror());
			break;
		}
		if (acl_vstream_writen(conn, buf, ret) != ret) {
			printf("fiber-%d: write error %s\r\n",
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
		printf("fiber-%d: listen error %s\r\n",
			acl_fiber_id(fiber), acl_last_serror());
		exit (1);
	}
	printf("fiber-%d: listen %s ok\r\n", acl_fiber_self(),
		ACL_VSTREAM_LOCAL(server));

	while (1) {
		ACL_VSTREAM *conn = acl_vstream_accept(server, NULL, 0);
		if (conn == NULL) {
			printf("accept error %s\r\n", acl_last_serror());
			if (acl_fiber_killed(fiber)) {
				printf("server fiber-%d: killed\r\n",
					acl_fiber_self());
				break;
			}
		}
		else
			printf("----accept one client ----\r\n");

		acl_fiber_create(fiber_client, conn, 32000);
	}

	acl_vstream_close(server);
}

static void fiber_killer(ACL_FIBER *fiber, void *ctx acl_unused)
{
	acl_fiber_sleep(1);

	printf("---kill(killer-%d, %p) fiber_wait1: fiber-%d---\r\n",
		acl_fiber_self(), fiber, acl_fiber_id(__fiber_wait1));

	acl_fiber_kill(__fiber_wait1);

	printf("---kill fiber_wait2: fiber-%d---\r\n",
		acl_fiber_id(__fiber_wait2));
	acl_fiber_kill(__fiber_wait2);

	acl_fiber_sleep(1);
	printf("---kill fiber_sleep: fiber-%d---\r\n",
		acl_fiber_id(__fiber_sleep));
	acl_fiber_kill(__fiber_sleep);

	acl_fiber_kill(__fiber_sleep2);

	acl_fiber_kill(__fiber_server);

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

	__fiber_wait1 = acl_fiber_create(fiber_wait, sem, 32000);
	__fiber_wait2 = acl_fiber_create(fiber_wait, sem, 32000);
	__fiber_sleep = acl_fiber_create(fiber_sleep, sem, 32000);
	__fiber_server = acl_fiber_create(fiber_server, NULL, 32000);

	__fiber_sleep2 = acl_fiber_create(fiber_sleep2, NULL, 32000);
	acl_fiber_create(fiber_killer, NULL, 32000);

	acl_fiber_schedule();
	acl_fiber_sem_free(sem);

	return 0;
}
