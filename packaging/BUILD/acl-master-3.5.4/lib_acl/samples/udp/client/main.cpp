#include "stdafx.h"
#include "udp.h"

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

static int sio(SOCK_UDP *sock, const char *data, int dlen, int count,
	int inter, bool need_read)
{
	int   i, ret;
	char  buf[4096];

	for (i = 0; i < count; i++) {
		ret = udp_send(sock, data, dlen);
		if (ret == -1) {
			printf("sock_write error %s\r\n", acl_last_serror());
			break;
		}

		if (need_read) {
			ret = udp_read(sock, buf, sizeof(buf) - 1);
			if (ret == -1) {
				printf("sock_read error %s\r\n",
					acl_last_serror());
				break;
			} else
				buf[ret] = 0;
			if (i % inter == 0)
				printf("result: %s\r\n", buf);
		}

		if (i % inter == 0) {
			snprintf(buf, sizeof(buf),
				"total: %d, curr: %d, dlen=%d",
				count, i, dlen);
			ACL_METER_TIME(buf);
		}
	}

	return i;
}

static int mio(SOCK_UDP *sock, const char *peer_addr, int dlen,
	int count, int inter, bool echo)
{
#define PKT_CNT	20
#define DAT_LEN 100

	char   bufs[PKT_CNT][DAT_LEN];
	SOCK_PKT pkts[PKT_CNT];

	int    n = 0, total_write = 0;

	if (dlen > DAT_LEN)
		dlen = DAT_LEN;

	for (int j = 0; j < PKT_CNT; j++) {
		memset(&bufs[j], 'X', dlen - 2);
		bufs[j][dlen - 1] = 0;
		udp_pkt_set_buf(&pkts[j], bufs[j], DAT_LEN);
		udp_pkt_set_peer(&pkts[j], peer_addr);
	}

	for (int i = 0; i < count; i++) {
		for (int j = 0; j < PKT_CNT; j++) {
			PKT_IOV_LEN(&pkts[j]) = DAT_LEN;
		}

		int nout = udp_msend(sock, pkts, PKT_CNT);
		if (nout < 0) {
			printf("sock_mwrite error %s\r\n", acl_last_serror());
			break;
		}

		total_write += nout;

		int nin = 0;
		if (echo) {
			nin = udp_mread(sock, pkts, PKT_CNT);
			if (nin < 0) {
				printf("sock_mread error %s\r\n", acl_last_serror());
				break;
			}
		}

		if (++n % inter == 0) {
			char buf[256], ip[64];
			snprintf(buf, sizeof(buf), "total: %d, curr: %d, "
				"nout: %d, nin: %d, dlen: %d, ip: %s, port: %d",
				total_write, n, nout, nin, dlen,
				udp_ip(sock, nin > 0 ? n % nin : 0,
					ip, sizeof(ip)),
				udp_port(sock, nin > 0 ? n % nin : 0));
			ACL_METER_TIME(buf);
		}
	}

	return n;
}

static void run(const char *local_addr, const char *peer_addr,
	int count, int dlen, int inter, int need_read, int mio_on, int quit)
{
	int       i;
	double    spent;
	char      data[4096];
	struct    timeval begin, end;
	SOCK_UDP *sock = udp_client_open(local_addr, peer_addr);

	(void) quit;

	if (sock == NULL) {
		printf("open_sock error!\r\n");
		return;
	}

	if (dlen > (int) sizeof(data) - 1)
		dlen = (int) sizeof(data) - 1;
	for (i = 0; i < dlen; i++)
		data[i] = 'X';
	data[dlen] = 0;

	gettimeofday(&begin, NULL);

	if (mio_on)
		i = mio(sock, peer_addr, dlen, count, inter, need_read);
	else
		i = sio(sock, data, dlen, count, inter, need_read);

	gettimeofday(&end, NULL);
	spent = stamp_sub(&end, &begin);

	printf("thread: %lu, total: %d, curr: %d, spent: %.2f, speed: %.2f\r\n",
		(unsigned long) acl_pthread_self(), count, i, spent,
		(i * 1000) / (spent > 1 ? spent : 1));

	printf("thread: %lu, peer addr: %s\r\n",
		(unsigned long) acl_pthread_self(), peer_addr);

	udp_close(sock);
}

typedef struct {
	char  local[64];
	char  peer[64];
	int   count;
	int   dlen;
	int   inter;
	int   need_read;
	int   mio_on;
	int   quit;
} THREAD_CTX;

static void thread_run(void *ctx)
{
	THREAD_CTX *tc = (THREAD_CTX*) ctx;

	run(tc->local, tc->peer, tc->count, tc->dlen,
		tc->inter, tc->need_read, tc->mio_on, tc->quit);
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
		"	-m if_mio_on[default: false]\r\n"
		"	-q if_send_quit when over [default: false]\r\n"
		"	-n loop_count [default: 1]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  peer[64], local[64];
	int   ch, count = 1, dlen = 100, inter = 1000, nthreads = 1, quit = 0;
	int   need_read = 0, mio_on = 0;

	acl_lib_init();
	acl_msg_stdout_enable(1);

	snprintf(peer, sizeof(peer), "127.0.0.1:8888");
	snprintf(local, sizeof(local), "127.0.0.1:0");

	while ((ch = getopt(argc, argv, "hl:s:n:N:i:t:rqm")) > 0) {
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
		case 'm':
			mio_on = 1;
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
			ctx->mio_on = mio_on;
			ctx->quit = quit;
			acl_pthread_pool_add(threads, thread_run, ctx);
		}

		acl_pthread_pool_destroy(threads);
	} else
		run(local, peer, count, dlen, inter, need_read, mio_on, quit);

	printf("\r\nlocal: %s, peer: %s, count: %d, dlen: %d, inter: %d\r\n",
		local, peer, count, dlen, inter);

	acl_lib_end();

	return 0;
}
