#include "lib_acl.h"
#include "lib_protocol.h"

static void get_url(const char *method, const char *url,
	const char *proxy, const char *dump)
{
	/* ���� HTTP ����ͷ */
	HTTP_HDR_REQ *hdr_req = http_hdr_req_create(url, method, "HTTP/1.1");
	ACL_VSTREAM *stream;
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	HTTP_HDR_RES *hdr_res;
	HTTP_RES *res;
	ACL_FILE *fp = NULL;
	const char *ptr;
	int   ret;

	/* ��� HTTP ����ͷ���� */

	http_hdr_print(&hdr_req->hdr, "---request hdr---");

	/* ����趨����������������Ӵ����������ַ��
	 * ����ʹ�� HTTP ����ͷ��ָ���ĵ�ַ
	 */

	if (*proxy)
		acl_vstring_strcpy(buf, proxy);
	else
		acl_vstring_strcpy(buf, http_hdr_req_host(hdr_req));

	/* ���Զ�� HTTP �����������ӵ�ַ */

	ptr = acl_vstring_memchr(buf, ':');
	if (ptr == NULL)
		acl_vstring_strcat(buf, ":80");
	else {
		int   port;
		ptr++;
		port = atoi(ptr);
		if (port <= 0 || port >= 65535) {
			printf("http server's addr(%s) invalid\n", acl_vstring_str(buf));
			acl_vstring_free(buf);
			http_hdr_req_free(hdr_req);
			return;
		}
	}

	/* ����Զ�� http ������ */

	stream = acl_vstream_connect(acl_vstring_str(buf) /* ��������ַ */,
			ACL_BLOCKING /* ����������ʽ */,
			10 /* ���ӳ�ʱʱ��Ϊ 10 �� */,
			10 /* ���� IO ������ʱʱ��Ϊ 10 �� */,
			4096 /* stream ����������СΪ 4096 �ֽ� */);
	if (stream == NULL) {
		printf("connect addr(%s) error(%s)\n",
			acl_vstring_str(buf), acl_last_serror());
		acl_vstring_free(buf);
		http_hdr_req_free(hdr_req);
		return;
	}

	/* ���� HTTP ����ͷ���� */

	http_hdr_build_request(hdr_req, buf);

	/* �� HTTP �������������� */

	ret = acl_vstream_writen(stream, acl_vstring_str(buf), ACL_VSTRING_LEN(buf));
	if (ret == ACL_VSTREAM_EOF) {
		printf("write to server error(%s)\n", acl_last_serror());
		acl_vstream_close(stream);
		acl_vstring_free(buf);
		http_hdr_req_free(hdr_req);
		return;
	}

	/* ����һ�� HTTP ��Ӧͷ���� */

	hdr_res = http_hdr_res_new();

	/* ��ȡ HTTP ��������Ӧͷ*/

	ret = http_hdr_res_get_sync(hdr_res, stream, 10 /* IO ��ʱʱ��Ϊ 10 �� */);
	if (ret < 0) {
		printf("get http reply header error(%s)\n", acl_last_serror());
		http_hdr_res_free(hdr_res);
		acl_vstream_close(stream);
		acl_vstring_free(buf);
		http_hdr_req_free(hdr_req);
		return;
	}

	if (http_hdr_res_parse(hdr_res) < 0) {
		printf("parse http reply header error\n");
		http_hdr_print(&hdr_res->hdr, "--- reply http header ---");
		http_hdr_res_free(hdr_res);
		acl_vstream_close(stream);
		acl_vstring_free(buf);
		http_hdr_req_free(hdr_req);
		return;
	}

	/* �����Ҫת������������Ҫ�ȴ��ļ� */

	if (dump != NULL) {
		fp = acl_fopen(dump, "w+");
		if (fp == NULL)
			printf("open file(%s) error(%s)\n",
				dump, acl_last_serror());
	}

	/* ��� HTTP ��Ӧû�������������� HTTP ��Ӧͷ���� */

	if (hdr_res->hdr.content_length == 0
		|| (hdr_res->hdr.content_length == -1
			&& !hdr_res->hdr.chunked
			&& hdr_res->reply_status > 300
			&& hdr_res->reply_status < 400))
	{
		if (fp)
			http_hdr_fprint(ACL_FSTREAM(fp), &hdr_res->hdr,
				"--- reply http header ---");
		else
			http_hdr_fprint(ACL_VSTREAM_OUT, &hdr_res->hdr,
				"--- reply http header ---");
		http_hdr_res_free(hdr_res);
		acl_vstream_close(stream);
		acl_vstring_free(buf);
		http_hdr_req_free(hdr_req);
		return;
	}

	/* ��� HTTP ��Ӧͷ */

	http_hdr_print(&hdr_res->hdr, "--- reply http header ---");

	/* ���� HTTP ��Ӧ����� */

	res = http_res_new(hdr_res);

	/* �������������ʼ��ȡ HTTP ��Ӧ�����岿�� */
	while (1) {
		http_off_t  n;
		char  buf2[4096];
		
		n = http_res_body_get_sync(res, stream, buf2, sizeof(buf2) - 1);
		if (n <= 0)
			break;

		if (fp) {
			if (acl_fwrite(buf2, (size_t) n, 1, fp) == (size_t) EOF) {
				printf("write to dump file(%s) error(%s)\n",
					dump, acl_last_serror());
				break;
			}
		} else {
			buf2[n] = 0;
			printf("%s", buf2);
		}
	}

	if (fp)
		acl_fclose(fp);
	http_res_free(res);  /* �ͷ� HTTP ��Ӧ����, hdr_res ���ڴ˺����ڲ��Զ����ͷ� */
	acl_vstream_close(stream);  /* �ر������� */
	acl_vstring_free(buf);  /* �ͷ��ڴ��� */
	http_hdr_req_free(hdr_req);  /* �ͷ� HTTP ����ͷ���� */
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -t method -r url -f dump_file -X proxy_addr\n"
		"example: %s -t GET -r http://www.sina.com.cn/ -f url_dump.txt\n",
		procname, procname);
}

int main(int argc, char *argv[])
{
	int   ch;
	char  url[256], dump[256], proxy[256], method[32];
	/*
	const char *url1 = "http://127.0.0.1:80/";
	HTTP_UTIL *req = http_util_req_new(url1, "POST");
	printf(">>>host: %s\n", http_hdr_req_host(req->hdr_req));
	acl_assert(req);
	return (0);
	*/

	acl_lib_init();  /* ��ʼ�� acl �� */

	ACL_SAFE_STRNCPY(method, "GET", sizeof(method));
	url[0] = 0;
	dump[0] = 0;
	proxy[0] = 0;
	while ((ch = getopt(argc, argv, "hr:t:f:X:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'r':
			ACL_SAFE_STRNCPY(url, optarg, sizeof(url));
			break;
		case 't':
			ACL_SAFE_STRNCPY(method, optarg, sizeof(method));
			break;
		case 'f':
			ACL_SAFE_STRNCPY(dump, optarg, sizeof(dump));
			break;
		case 'X':
			ACL_SAFE_STRNCPY(proxy, optarg, sizeof(proxy));
			break;
		default:
			break;
		}
	}

	if (url[0] == 0) {
		usage(argv[0]);
		return (0);
	}

	get_url(method, url, proxy, dump);
	return (0);
}
