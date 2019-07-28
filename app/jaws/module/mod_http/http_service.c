#include "lib_acl.h"

#include "dns_lookup.h"
#include "service.h"
#include "http_module.h"
#include "http_vhost.h"
#include "http_service.h"

#ifdef ACL_MS_WINDOWS
# include <process.h>
# pragma comment(lib,"ws2_32")
# define getpid _getpid
#endif

void http_service_start(HTTP_CLIENT *http_client)
{
	switch (http_filter_type()) {
	case HTTP_FILTER_PROXY:
		http_proxy_start(http_client);
		break;
	case HTTP_FILTER_HTTPD:
		http_server_start(http_client);
		break;
	default:
		forward_complete(&http_client->entry);
		break;
	}
}

void http_service_main(HTTP_SERVICE *service, ACL_ASTREAM *stream)
{
	HTTP_CLIENT *client;

	client = http_client_new(service, stream);
	http_service_start(client);
}

void http_service_free(HTTP_SERVICE *service)
{
	http_plugin_unload_all();
	service_free((SERVICE*) service);
}

HTTP_SERVICE *http_service_new()
{
	HTTP_SERVICE *service;

	service = (HTTP_SERVICE *) service_alloc("http", sizeof(HTTP_SERVICE));
	service->file_path = acl_vstring_alloc(256);

	/* 将动态插件的回调函数赋给服务对象 */
	http_plugin_set_callback(service);

	/* 针对HTTP服务器运行模式，初始化文件缓存 */
	file_cache_init();

	return (service);
}
