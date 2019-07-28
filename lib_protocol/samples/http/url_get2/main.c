#include "lib_acl.h"
#include "lib_protocol.h"

static void get_url(const char *method, const char *url,
	const char *proxy, const char *dump, int out)
{
	/* 创建 HTTP_UTIL 请求对象 */
	HTTP_UTIL *http = http_util_req_new(url, method);
	int   ret;

	/* 如果设定代理服务器，则连接代理服务器地址，
	 * 否则使用 HTTP 请求头里指定的地址
	 */

	if (proxy && *proxy)
		http_util_set_req_proxy(http, proxy);

	/* 设置转储文件 */

	if (dump && *dump)
		http_util_set_dump_file(http, dump);

	/* 输出 HTTP 请求头内容 */

	http_hdr_print(&http->hdr_req->hdr, "---request hdr---");

	/* 连接远程 http 服务器 */

	if (http_util_req_open(http) < 0) {
		printf("open connection(%s) error\n", http->server_addr);
		http_util_free(http);
		return;
	}

	/* 读取 HTTP 服务器响应头*/

	ret = http_util_get_res_hdr(http);
	if (ret < 0) {
		printf("get reply http header error\n");
		http_util_free(http);
		return;
	}

	/* 输出 HTTP 响应头 */

	http_hdr_print(&http->hdr_res->hdr, "--- reply http header ---");

	/* 如果有数据体则开始读取 HTTP 响应数据体部分 */

	while (1) {
		char  buf[4096];
		
		ret = http_util_get_res_body(http, buf, sizeof(buf) - 1);
		if (ret <= 0)
			break;
		buf[ret] = 0;
		if (out)
			printf("%s", buf);
	}
	http_util_free(http);
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -t method -r url -f dump_file -o[output] -X proxy_addr\n"
		"example: %s -t GET -r http://www.sina.com.cn/ -f url_dump.txt\n",
		procname, procname);
}

int main(int argc, char *argv[])
{
	int   ch, out = 0;
	char  url[256], dump[256], proxy[256], method[32];

	acl_lib_init();  /* 初始化 acl 库 */

	ACL_SAFE_STRNCPY(method, "GET", sizeof(method));
	url[0] = 0;
	dump[0] = 0;
	proxy[0] = 0;
	while ((ch = getopt(argc, argv, "hor:t:f:X:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'o':
			out = 1;
			break;
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

	get_url(method, url, proxy, dump, out);
	return (0);
}
