#include "stdafx.h"

static double stamp_sub(const struct timeval *from, const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return res.tv_sec * 1000.0 + res.tv_usec/1000.0;
}

static void run(const char *local_addr, const char *peer_addr,
	int count, int dlen, int inter, int need_read, int quit)
{
	double spent;
	int   ret, i;
	char  buf[4096], data[4096];
	struct timeval begin, end;
	ACL_VSTREAM *stream = acl_vstream_bind(local_addr, 2, 0);  /* 绑定 UDP 套接口 */

	if (stream == NULL) {
		printf("acl_vstream_bind %s error %s\r\n",
			local_addr, acl_last_serror());
		return;
	}

	if (dlen > (int) sizeof(data) - 1)
		dlen = (int) sizeof(data) - 1;
	for (i = 0; i < dlen; i++)
		data[i] = 'X';
	data[dlen] = 0;

	gettimeofday(&begin, NULL);
	acl_vstream_set_peer(stream, peer_addr);
	ACL_VSTREAM_SET_RWTIMO(stream, 1);

	for (i = 0; i < count; i++) {
		/* 如果服务端的地址是变化的，则应该在写每次前都需要调用
		 * acl_vstream_set_peer
		 */
		ret = acl_vstream_write(stream, data, dlen);
		if (ret == ACL_VSTREAM_EOF) {
			printf("acl_vtream_write error %s\r\n",
				acl_last_serror());
			break;
		}

		if (need_read) {
			ret = acl_vstream_read(stream, buf, sizeof(buf) - 1);
			if (ret == ACL_VSTREAM_EOF) {
				if (errno == ETIMEDOUT) {
					printf("timeout read\r\n");
					continue;
				}
				printf("acl_vstream_read error %s\r\n",
						acl_last_serror());
				break;
			} else
				buf[ret] = 0;
			if (i % inter == 0)
				printf("result: %s\r\n", buf);
		}

		if (i % inter == 0) {
			snprintf(buf, sizeof(buf), "total: %d, curr: %d",
				count, i);
			ACL_METER_TIME(buf);
		}
	}

	gettimeofday(&end, NULL);
	spent = stamp_sub(&end, &begin);

	printf("thread: %lu, total: %d, curr: %d, spent: %.2f, speed: %.2f\r\n",
		(unsigned long) acl_pthread_self(), count, i, spent,
		(i * 1000) / (spent > 1 ? spent : 1));

	printf("thread: %lu, local addr: %s, peer addr: %s\r\n",
		(unsigned long) acl_pthread_self(), ACL_VSTREAM_LOCAL(stream),
		ACL_VSTREAM_PEER(stream));

	if (quit)
		acl_vstream_write(stream, "quit", 4);
	acl_vstream_close(stream);
}

typedef struct {
	char  local[64];
	char  peer[64];
	int   count;
	int   dlen;
	int   inter;
	int   need_read;
	int   quit;
} THREAD_CTX;

static void thread_run(void *ctx)
{
	THREAD_CTX *tc = (THREAD_CTX*) ctx;

	run(tc->local, tc->peer, tc->count, tc->dlen,
		tc->inter, tc->need_read, tc->quit);
	acl_myfree(ctx);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-s server_addr [default: 127.0.0.1:8888]\r\n"
		"	-l local_addr [default: 127.0.0.1:0]\r\n"
		"	-t thread_count [default: 1]\r\n"
		"	-i print_per_loop [default: 1000]\r\n"
		"	-N data_len [default: 100]\r\n"
		"	-r if_need_read [default: false]\r\n"
		"	-q if_send_quit when over [default: false]\r\n"
		"	-n loop_count [default: 1]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  peer[64], local[64];
	int   ch, count = 1, dlen = 100, inter = 1000, nthreads = 1, quit = 0;
	int   need_read = 0;

	acl_lib_init();
	acl_msg_stdout_enable(1);

	snprintf(peer, sizeof(peer), "127.0.0.1:8888");
	snprintf(local, sizeof(local), "127.0.0.1:0");

	while ((ch = getopt(argc, argv, "hl:s:n:N:i:t:rq")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'l':
			snprintf(local, sizeof(local), "%s", optarg);
			break;
		case 's':
			snprintf(peer, sizeof(peer), "%s", optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'N':
			dlen = atoi(optarg);
			break;
		case 'i':
			inter = atoi(optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'r':
			need_read = 1;
			break;
		case 'q':
			quit = 1;
			break;
		default:
			break;
		}
	}

	if (peer[0] == 0 || local[0] == 0) {
		usage(argv[0]);
		return 1;
	}

	if (nthreads > 1) {
		int   i;
		acl_pthread_pool_t *threads =
			acl_thread_pool_create(nthreads, 120);
		for (i = 0; i < nthreads; i++) {
			THREAD_CTX *ctx = (THREAD_CTX*)
				acl_mymalloc(sizeof(THREAD_CTX));
			snprintf(ctx->local, sizeof(ctx->local), "%s", local);
			snprintf(ctx->peer, sizeof(ctx->peer), "%s", peer);
			ctx->count = count;
			ctx->dlen = dlen;
			ctx->inter = inter;
			ctx->need_read = need_read;
			ctx->quit = quit;
			acl_pthread_pool_add(threads, thread_run, ctx);
		}

		acl_pthread_pool_destroy(threads);
	} else
		run(local, peer, count, dlen, inter, need_read, quit);

	printf("\r\nlocal: %s, peer: %s, count: %d, dlen: %d, inter: %d\r\n",
		local, peer, count, dlen, inter);

	acl_lib_end();

	return 0;
}
