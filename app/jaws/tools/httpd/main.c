#include "lib_acl.h"
#include "lib_protocol.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	ACL_VSTREAM *stream;
	int  nconn;
} CONN;

static void init(void)
{
	acl_init();
}

static void end(void)
{
	acl_end();
}

static void thread_run(void *arg)
{
	CONN *conn = (CONN*) arg;
	ACL_VSTREAM *client = conn->stream;
	const char *keep_reply_200 = "HTTP/1.1 200 OK\r\n"
		"Server: nginx/0.6.32\r\n"
		"Date: Tue, 29 Dec 2009 02:18:25 GMT\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 43\r\n"
		"Last-Modified: Mon, 16 Nov 2009 02:18:14 GMT\r\n"
		"Connection: keep-alive\r\n"
		"Accept-Ranges: bytes\r\n\r\n"
		"<html>\n"
		"<body>\n"
		"hello world!\n"
		"</body>\n"
		"</html>\n";
	const char *close_reply_200 = "HTTP/1.1 200 OK\r\n"
		"Server: nginx/0.6.32\r\n"
		"Date: Tue, 29 Dec 2009 02:18:25 GMT\r\n"
		"Content-Type: text/html\r\n"
		"Last-Modified: Mon, 16 Nov 2009 02:18:14 GMT\r\n"
		"Connection: close\r\n"
		"Accept-Ranges: bytes\r\n\r\n";

	int   ret, keep_alive;

	/*
	if (conn->nconn % 2 == 0) {
		acl_vstream_close(client);
		acl_myfree(conn);
		return;
	}
	*/

	while (1) {
		HTTP_REQ *req;
		HTTP_HDR_REQ *hdr_req = http_hdr_req_new();
		char  buf[4096];

		ret = http_hdr_req_get_sync(hdr_req, client, 300);
		if (ret < 0) {
			http_hdr_req_free(hdr_req);
			break;
		}
		if (http_hdr_req_parse(hdr_req) < 0) {
			http_hdr_req_free(hdr_req);
			printf("parse error\n");
			break;
		}

		keep_alive = hdr_req->hdr.keep_alive;

		if (hdr_req->hdr.content_length > 0) {
			req = http_req_new(hdr_req);
			ret = (int) http_req_body_get_sync(req, client, buf, sizeof(buf));
			if (ret < 0) {
				http_req_free(req);
				break;
			}
			http_req_free(req);
		} else {
			http_hdr_req_free(hdr_req);
		}

		ret = acl_vstream_writen(client, keep_reply_200, strlen(keep_reply_200));
		break;

		if (conn->nconn % 3 == 0) {
			ret = acl_vstream_writen(client, close_reply_200, strlen(close_reply_200));
			if (ret == ACL_VSTREAM_EOF)
				break;
			memset(buf, 'X', sizeof(buf));
			while (1) {
				ret = acl_vstream_writen(client, buf, sizeof(buf));
				if (ret == ACL_VSTREAM_EOF) {
					printf("write error\n");
					goto END;
				}
			}
		} else {
			ret = acl_vstream_writen(client, keep_reply_200, strlen(keep_reply_200));
			if (ret == ACL_VSTREAM_EOF)
				break;
			if (!keep_alive)
				break;
			conn->nconn++;
		}
	}

END:
	acl_vstream_close(client);
	acl_myfree(conn);
}
	
static void run_server(const char *addr)
{
	ACL_VSTREAM *sstream = acl_vstream_listen(addr, 128);
	acl_pthread_pool_t *pool;
	CONN *conn;
	int   n = 0;

	if (sstream == NULL) {
		acl_msg_error("listen %s error(%s)", addr, acl_last_serror());
		return;
	}

	printf("listening on %s ...\n", addr);
	pool = acl_thread_pool_create(100, 120);
	while (1) {
		ACL_VSTREAM *client = acl_vstream_accept(sstream, NULL, 0);
		if (client == NULL) {
			acl_msg_error("accept error(%s)", acl_last_serror());
			break;
		}
/*
printf("accept one: %d\n", ACL_VSTREAM_SOCK(client));
shutdown(ACL_VSTREAM_SOCK(client), SHUT_RD | SHUT_WR);
continue;
*/
		conn = acl_mycalloc(1, sizeof(CONN));
		conn->stream = client;
		conn->nconn = n++;
		acl_pthread_pool_add(pool, thread_run, conn);
	}

	acl_vstream_close(sstream);
}

static void run_client(const char *addr, const char *filename)
{
	char *request = acl_vstream_loadfile(filename);
	ACL_VSTREAM *client;
	int   ret;
	char  buf[1024];

	if (request == NULL) {
		printf("load file(%s) error(%s)\n", filename, acl_last_serror());
		return;
	}

	client = acl_vstream_connect(addr, ACL_BLOCKING,
			0, 0, 4096);
	if (client == NULL) {
		printf("connect addr(%s) error(%s)\n", addr, acl_last_serror());
		acl_myfree(request);
		return;
	}

	acl_tcp_set_sndbuf(ACL_VSTREAM_SOCK(client), 10);
	if (acl_vstream_writen(client, request, strlen(request)) == ACL_VSTREAM_EOF) {
		printf("write to addr(%s) error(%s)\n", addr, acl_last_serror());
		acl_vstream_close(client);
		acl_myfree(request);
		return;
	}

	memset(buf, 0, sizeof(buf));

	while (1) {
		ret = acl_vstream_read(client, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF)
			break;
		buf[ret] = 0;
		usleep(100000);
		printf(">>>%s\n", buf);
	}

	printf(">>>last data(%s)\n", buf);
	acl_vstream_close(client);
	acl_myfree(request);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -s listen_addr[IP:PORT] -c request_hdr\n", procname);
}

int main(int argc, char *argv[])
{
	char addr[256] = "0.0.0.0:80", filename[256];
	int  ch, client_mode = 0;

	while ((ch = getopt(argc, argv, "hs:c:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			exit (0);
		case 's':
			ACL_SAFE_STRNCPY(addr, optarg, sizeof(addr));
			break;
		case 'c':
			client_mode = 1;
			ACL_SAFE_STRNCPY(filename, optarg, sizeof(filename));
			break;
		default:
			break;
		}
	}

	init();
	if (client_mode)
		run_client(addr, filename);
	else
		run_server(addr);
	end();
	return (0);
}
