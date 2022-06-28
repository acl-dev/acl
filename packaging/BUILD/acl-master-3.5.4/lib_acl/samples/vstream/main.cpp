#include "lib_acl.h"
#include <assert.h>
#ifdef	ACL_UNIX
#include <signal.h>
#endif

static void end(void)
{
#ifdef ACL_MS_WINDOWS
	acl_vstream_fprintf(ACL_VSTREAM_OUT, "enter any key to quit\r\n");
	getchar();
#endif
}

static char addr[256] = "127.0.0.1:8285";
static int  timeout = 0;

static void reply_client2(ACL_VSTREAM *client)
{
	char  buf[1024];
	int   ret;

	while (1) {
		ret = acl_vstream_gets(client, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF)
			break;
		buf[ret] = 0;
		acl_vstream_printf("get one line:%s", buf);
		if (acl_vstream_writen(client, buf, ret) == ACL_VSTREAM_EOF)
			break;
	}
}

static void reply_client(ACL_VSTREAM *client)
{
#define	COUNT	10
#define	SIZE	8192000
	const char *myname = "reply_client";
	struct iovec vector[COUNT];
	int   i, dlen = 0, n;

	for (i = 0; i < COUNT; i++) {
#ifdef MINGW
		vector[i].iov_base = (char*) acl_mycalloc(1,  SIZE);
#else
		vector[i].iov_base = acl_mycalloc(1,  SIZE);
#endif
		assert(vector[i].iov_base);
		vector[i].iov_len = SIZE;
		dlen += SIZE;
	}

	printf("%s: before write\n", myname);
	n = acl_vstream_writevn(client, vector, COUNT);

	for (i = 0; i < COUNT; i++) {
		acl_myfree(vector[i].iov_base);
	}

	printf("%s: writen n = %d, dlen = %d\r\n", myname, n, dlen);
}

static int vstream_server(void)
{
	const char *myname = "vstream_server";
	ACL_VSTREAM *sstream, *client;
	char  ebuf[256];

	sstream = acl_vstream_listen(addr, 128);
	if (sstream == NULL) {
		printf("%s(%d): listen on %s error(%s)\r\n",
			myname, __LINE__, addr,
			acl_last_strerror(ebuf, sizeof(ebuf)));
		return (-1);
	}

	acl_tcp_defer_accept(ACL_VSTREAM_SOCK(sstream), 0);
	printf("%s: listen %s ok\r\n", myname, addr);
	while (1) {
		client = acl_vstream_accept(sstream, NULL, 0);
		if (client == NULL) {
			printf("%s(%d): accept error(%s)\r\n",
				myname, __LINE__,
				acl_last_strerror(ebuf, sizeof(ebuf)));
			return (-1);
		}
		printf("accept one\r\n");
		if (1)
			reply_client2(client);
		else
			reply_client(client);
		acl_vstream_close(client);
	}
	
	return (0);
}

static int vstream_client(void)
{
	const char *myname = "vstream_client";
	ACL_VSTREAM *client;
	char  ebuf[256], buf[4096];
	int   n, dlen = 0;

	printf("addr: %s, timeout: %d\n", addr, timeout);
	client = acl_vstream_connect(addr, ACL_NON_BLOCKING, timeout, timeout, 1024);
	if (client == NULL) {
		printf("%s(%d): connect addr %s error(%s)\n",
			myname, __LINE__, addr, acl_last_strerror(ebuf, sizeof(ebuf)));
		return (-1);
	}

	printf("%s: connect %s ok\r\n", myname, addr);
	acl_non_blocking(ACL_VSTREAM_SOCK(client), ACL_BLOCKING);

	n = acl_write_wait(ACL_VSTREAM_SOCK(client), 10);
	if (n < 0) {
		printf("connect timeout: %s\n", acl_last_serror());
		goto END;
	}
	acl_vstream_fprintf(client, "hello world\n");
	while (1) {
		n = acl_vstream_read(client, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF) {
			printf("read over: %s\n", acl_last_serror());
			break;
		}
		dlen += n;
		buf[n] = 0;
		printf("read reply: %s\n", buf);
	}

END:
	acl_vstream_close(client);
	printf("%s: read %d\n", myname, dlen);
	return (0);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -a [addr] -s [server_mode] -c [client_mode] -t [timeout]\r\n", procname);
}

#ifdef WIN32
#define snprintf _snprintf
#endif

int main(int argc acl_unused, char *argv[] acl_unused)
{
	char  buf[256];
	int   i, c;
	ACL_VSTRING *sbuf;
	int   flag = 0;

	acl_lib_init();

#if 0
	return (vstream_server());
#endif

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			continue;
		switch (argv[i][1]) {
		case 's':
			flag = 1;
			break;
		case 'c':
			flag = 2;
			break;
		case 'a':
			snprintf(addr, sizeof(addr), "%s", argv[i + 1]);
			break;
		case 't':
			timeout = atoi(argv[i + 1]);
			break;
		case 'h':
			usage(argv[0]);
			exit (0);
		default:
			break;
		}
	}

	if (flag == 1)
		return (vstream_server());
	if (flag == 2)
		return (vstream_client());

	sbuf = acl_vstring_alloc(256);
	while (1) {
		acl_vstream_fprintf(ACL_VSTREAM_OUT, "please input(quit on exit): ");
		acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		acl_vstream_fprintf(ACL_VSTREAM_ERR, "your input: %s\r\n", buf);
		if (strcasecmp(buf, "quit") == 0 || strcasecmp(buf, "exit") == 0)
			break;

		acl_vstream_printf("please input: ");
		c = acl_vstring_gets_nonl_bound(sbuf, ACL_VSTREAM_IN, 10);
		if (c == ACL_VSTREAM_EOF)
			break;
		acl_vstream_printf("your input(len=%d): %s\r\n",
			ACL_VSTRING_LEN(sbuf), acl_vstring_str(sbuf));
		if (c == '\n')
			continue;
		if (acl_vstring_gets_nonl(sbuf, ACL_VSTREAM_IN) == '\n')
			acl_vstream_printf("the rest of your input(len=%d): %s\r\n",
				ACL_VSTRING_LEN(sbuf), acl_vstring_str(sbuf));
		else
			break;
	}
	acl_vstring_free(sbuf);
	end();
	return (0);
}
