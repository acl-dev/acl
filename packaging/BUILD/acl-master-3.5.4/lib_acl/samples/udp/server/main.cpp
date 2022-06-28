#include "stdafx.h"
#include "udp.h"

static void sio(SOCK_UDP *sock, int inter, bool echo)
{
	int   i = 0, ret;
	char  buf[4096];

	while (true) {
		ret = udp_read(sock, buf, sizeof(buf) - 1);
		if (ret == -1) {
			printf("sock_read error %s\r\n", acl_last_serror());
			break;
		} else
			buf[ret] = 0;

//		if (++i % inter == 0)
			printf("result: %s\r\n", buf);

		if (echo && (ret = udp_send(sock, buf, ret)) == -1) {
			printf("sock_write error %s\r\n", acl_last_serror());
			break;
		}

		if (i % inter == 0) {
			snprintf(buf, sizeof(buf), "curr: %d, dlen=%d", i, ret);
			ACL_METER_TIME(buf);
		}
	}
}

static void mio(SOCK_UDP *sock, int inter, bool echo)
{
#define PKT_CNT	20
#define BUF_LEN 1024

	char   bufs[PKT_CNT][BUF_LEN];
	SOCK_PKT pkts[PKT_CNT];

	int    i, nread = 0, nwrite = 0, total_read = 0, n = 0;

	while (true) {
		for (i = 0; i < PKT_CNT; i++) {
			bufs[i][0] = 0;
			udp_pkt_set_buf(&pkts[i], bufs[i], BUF_LEN);
		}

		int ret = udp_mread(sock, pkts, PKT_CNT);
		if (ret <= 0) {
			printf(">>read error ret: %d, errno: %d, %d\r\n",
				ret, errno, EMSGSIZE);
			break;
		}

		nread++;
		total_read += ret;

		for (i = 0; i < ret; i++) {
			PKT_IOV_LEN(&pkts[i]) = SOCK_PKT_LEN(sock, i);
#if 0
			printf("[%s], addr_len: %d, %lu\r\n",
				(char *) PKT_IOV_DAT(&pkts[i]),
				(int) pkts[i].addr_len, sizeof(pkts[i].addr));
			struct sockaddr_in *in = (struct sockaddr_in *)
				sock->msgvec[i].msg_hdr.msg_name;
			printf("ip: %s, port: %d\r\n",
				inet_ntoa(in->sin_addr), ntohs(in->sin_port));
#endif
		}

		if (echo) {
			ret = udp_msend(sock, pkts, ret);
			if (ret <= 0) {
				printf("sock_mwrite error %s, ret: %d\r\n",
					acl_last_serror(), ret);
				printf("len: %d\r\n", SOCK_PKT_LEN(sock, 0));
				break;
			}
			nwrite++;
		}

		if (++n % inter == 0) {
			char buf[256], ip[64], k;

			k = ret > 0 ? time(NULL) % ret : 0;

			snprintf(buf, sizeof(buf),
				"curr: %d, total: %d, ret: %d, dlen: %ld, "
				"nread: %d, nwrite: %d, ip: %s, port-%d: %d",
				i, total_read, ret, PKT_IOV_LEN(&pkts[0]),
				nread, nwrite,
				udp_ip(sock, k, ip, sizeof(ip)),
				k, udp_port(sock, k));
			ACL_METER_TIME(buf);
		}
	}
}

static void run(const char *local_addr, int inter, int mio_on, int need_echo)
{
	SOCK_UDP *sock = udp_server_open(local_addr);

	if (sock == NULL) {
		printf("open_sock error!\r\n");
		return;
	}

	if (mio_on)
		mio(sock, inter, need_echo);
	else
		sio(sock, inter, need_echo);

	udp_close(sock);
}

typedef struct {
	char  local[64];
	int   inter;
	int   mio_on;
	int   need_echo;
} THREAD_CTX;

static void thread_run(void *ctx)
{
	THREAD_CTX *tc = (THREAD_CTX*) ctx;

	run(tc->local, tc->inter, tc->mio_on, tc->need_echo);
	acl_myfree(ctx);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-s server_addr [default: 127.0.0.1:8888]\r\n"
		"	-t thread_count [default: 1]\r\n"
		"	-i print_per_loop [default: 1000]\r\n"
		"	-m if_mio_on[default: false]\r\n"
		"	-e if_echo [default: false]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  local[64];
	int   ch, inter = 1000, nthreads = 1;
	int   need_echo = 0, mio_on = 0;

	acl_lib_init();
	acl_msg_stdout_enable(1);

	snprintf(local, sizeof(local), "127.0.0.1:8888");

	while ((ch = getopt(argc, argv, "hs:i:t:em")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(local, sizeof(local), "%s", optarg);
			break;
		case 'i':
			inter = atoi(optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'e':
			need_echo = 1;
			break;
		case 'm':
			mio_on = 1;
			break;
		default:
			break;
		}
	}

	if (local[0] == 0) {
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
			ctx->inter = inter;
			ctx->need_echo = need_echo;
			ctx->mio_on = mio_on;
			acl_pthread_pool_add(threads, thread_run, ctx);
		}

		acl_pthread_pool_destroy(threads);
	} else
		run(local, inter, mio_on, need_echo);

	acl_lib_end();

	return 0;
}
