#include "lib_acl.h"
#include <time.h>
#ifdef	ACL_UNIX
#include <pthread.h>
#endif

typedef struct THREAD_CTX 
{
	int   count;
	char  ip[64];
	unsigned short port;
	char  domain[256];
	time_t tmspec;
} THREAD_CTX;

static char *var_server_ip = "127.0.0.1";
static int   var_server_port = 53;
static char *var_domain = "mailz.hexun.com";
static acl_pthread_mutex_t __lock;

static int __total_cnt = 0, __count = 0;

static void bench_info(int count, time_t tmspec)
{
	printf("total_cnt=%d, count=%d, tmspec=%ld\r\n",
		__total_cnt, count, tmspec);
}

static void bench_stat(int cnt)
{
	static time_t last;
	time_t now, inter;

	return;

	acl_pthread_mutex_lock(&__lock);
	__total_cnt += cnt;
	__count += cnt;
	time(&now);
	inter = now - last;
	if (inter >= 1) {
		bench_info(__count, inter);
		__count = 0;
		last = now;
	}
	acl_pthread_mutex_unlock(&__lock);
}

static void init(void)
{
	acl_socket_init();
	acl_pthread_mutex_init(&__lock, NULL);
}

static void *thread_run(void *arg)
{
	THREAD_CTX *ctx = (THREAD_CTX*) arg;
	ACL_RES *res;
	ACL_DNS_DB *dns_db;
	time_t begin, last;
	int   i, cnt;

	res = acl_res_new(ctx->ip, ctx->port);

	last = begin = time(NULL);
	cnt = 0;
	for (i = 0; i < ctx->count; i++)
	{
		dns_db = acl_res_lookup(res, ctx->domain);
		acl_netdb_free(dns_db);
		cnt++;
		if (i % 4000 == 0) {
			bench_stat(cnt);
			cnt = 0;
			last = time(NULL);
		}
	}

	bench_stat(cnt);
	bench_info(cnt, time(NULL) - last);
	acl_res_free(res);
	ctx->tmspec = time(NULL) - begin;
	return (ctx);
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -t {thread_count} -n {total_count} -i {server_ip} -p {server_port}\r\n",
		procname);
}

int main(int argc, char *argv[])
{
	char  ch;
	int   nthread = 50, count = 1000000, i, n;
	time_t tmspec;
	acl_pthread_t *ptid;
	acl_pthread_attr_t attr;
	THREAD_CTX *ctx;
	time_t begin;
	void *ptr;

	while ((ch = getopt(argc, argv, "ht:n:i:p:")) > 0)
	{
		switch (ch) {
		case 'h':
			usage(argv[0]);
			exit (0);
		case 't':
			nthread = atol(optarg);
			break;
		case 'n':
			count = atol(optarg);
			break;
		case 'i':
			var_server_ip = acl_mystrdup(optarg);
			break;
		case 'p':
			var_server_port = atoi(optarg);
			break;
		default:
			break;
		}
	}

	init();

	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, 0);

	begin = time(NULL);
	n = count / nthread;
	ptid = (acl_pthread_t*) acl_mycalloc(nthread, sizeof(acl_pthread_t));
	for (i = 0; i < nthread; i++)
	{
		ctx = (THREAD_CTX*) acl_mycalloc(1, sizeof(THREAD_CTX));
		ctx->count = n;
		ACL_SAFE_STRNCPY(ctx->ip, var_server_ip, sizeof(ctx->ip));
		ctx->port = var_server_port;
		ACL_SAFE_STRNCPY(ctx->domain, var_domain, sizeof(ctx->domain));
		acl_pthread_create(&ptid[i], &attr, thread_run, (void*) ctx);
	}

	tmspec = 0;
	for (i = 0; i < nthread; i++)
	{
		acl_pthread_join(ptid[i], &ptr);
		ctx = (THREAD_CTX*) ptr;
		tmspec += ctx->tmspec;
		acl_myfree(ctx);
	}
	acl_myfree(ptid);
	printf("total time spent: %ld, inter spent time: %ld secconds\r\n",
		tmspec, time(NULL) - begin);
	printf("Enter any key to exit\r\n");
	getchar();
	return (0);
}
