#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

static void* thread_main(void* ctx)
{
	time_t begin, now;
	const char* user = (const char* ) ctx;

	if (user && *user) {
		struct passwd* pwd = getpwnam(user);
		if (pwd == NULL) {
			printf("getpwnam error=%s, user=$%s\r\n",
				strerror(errno), user);
			return NULL;
		}
		if (setgid(pwd->pw_gid) < 0) {
			printf("setgid error=%s\r\n", strerror(errno));
			return NULL;
		}
		if (initgroups(user, pwd->pw_gid) < 0) {
			printf("initgroups error=%s\r\n", strerror(errno));
			return NULL;
		}

		if (setuid(pwd->pw_uid) < 0) {
			printf("setuid error=%s\r\n", strerror(errno));
			return NULL;
		}
		printf("thread-%lu setuid ok\r\n", pthread_self());
	}

	printf("thread-%lu: sleep a while\r\n", pthread_self());
	time(&begin);

	while ((now = time(NULL)) <= begin + 5) {
		sleep(1);
	}

	printf("thread-%lu: my uid=%ld, gid=%ld, will exit\r\n",
		(long) getuid(), (long) getgid(), pthread_self());
	return NULL;
}

#define MAX	10

int main(int argc, char* argv[])
{
	char* user = "nobody";
	pthread_t threads[MAX], tid;
	int   i;

	if (argc >= 2) {
		user = argv[1];
	}

	for (i = 0; i < MAX; i++) {
		pthread_create(&threads[i], NULL, thread_main, NULL);
	}
	sleep(1);
	pthread_create(&tid, NULL, thread_main, user);

	for (i = 0; i < MAX; i++) {
		pthread_join(threads[i], NULL);
	}
	pthread_join(tid, NULL);
	return 0;
}
