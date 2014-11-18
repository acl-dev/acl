#include "lib_acl.h"
#include "http_service.h"

static ACL_FIFO *__handles = NULL;

static ACL_FIFO __request_plugins;
static ACL_FIFO __request_dat_plugins;
static ACL_FIFO __respond_plugins;
static ACL_FIFO __respond_dat_plugins;

void http_plugin_load(ACL_DLL_ENV *dll_env, const char *dlname, const char *plugin_cfgdir)
{
	const char *myname = "http_plugin_load";
	ACL_DLL_HANDLE handle;
	HTTP_PLUGIN *http_plugin;
	HTTP_PLUGIN  plugin;

	if (dlname == NULL || *dlname == 0)
		return;

	acl_msg_info("%s(%d): begin load %s now ...", __FUNCTION__, __LINE__, dlname);
	handle = acl_dlopen(dlname);
	if (handle == NULL) {
		acl_msg_error("%s(%d): load %s error(%s)",
			myname, __LINE__, dlname, acl_last_serror());
		return;
	}

	acl_msg_info("%s(%d): load %s ok.", __FUNCTION__, __LINE__, dlname);

	if (__handles == NULL) {
		__handles = acl_fifo_new();
	}

	acl_fifo_push(__handles, handle);

	/* 初始化 */
	plugin.init = (plugin_init_fn) (intptr_t)
		acl_dlsym(handle, "http_plugin_init");
	if (plugin.init) {
		plugin.init(dll_env, plugin_cfgdir);
	}

	/* 添加请求头过滤器 */

	plugin.filter.request = (plugin_filter_request_fn) (intptr_t)
		acl_dlsym(handle, "http_request_filter");
	plugin.forward.request = (plugin_forward_request_fn) (intptr_t)
		acl_dlsym(handle, "http_request_forward");
	if (plugin.filter.request) {
		http_plugin = (HTTP_PLUGIN*) acl_mycalloc(1, sizeof(HTTP_PLUGIN));
		http_plugin->filter.request = plugin.filter.request;
		http_plugin->forward.request = plugin.forward.request;
		acl_fifo_push(&__request_plugins, http_plugin);
	}

	/* 添加请求体过滤器 */

	plugin.data_free = (plugin_dat_free_fn) (intptr_t)
		acl_dlsym(handle, "http_request_dat_free");
	plugin.data_filter = (plugin_dat_filter_fn) (intptr_t)
		acl_dlsym(handle, "http_request_dat_filter");
	if (plugin.data_filter) {
		http_plugin = (HTTP_PLUGIN*) acl_mycalloc(1, sizeof(HTTP_PLUGIN));
		http_plugin->data_filter = plugin.data_filter;
		http_plugin->data_free = plugin.data_free;
		acl_fifo_push(&__request_dat_plugins, http_plugin);
	}

	/* 添加响应头过滤器 */

	plugin.filter.respond = (plugin_filter_respond_fn) (intptr_t)
		acl_dlsym(handle, "http_respond_filter");
	plugin.forward.respond = (plugin_forward_respond_fn) (intptr_t)
		acl_dlsym(handle, "http_respond_forward");
	if (plugin.filter.respond) {
		http_plugin = (HTTP_PLUGIN*) acl_mycalloc(1, sizeof(HTTP_PLUGIN));
		http_plugin->filter.respond = plugin.filter.respond;
		http_plugin->forward.respond = plugin.forward.respond;
		acl_fifo_push(&__respond_plugins, http_plugin);
	}

	/* 添加响应体过滤器 */

	plugin.data_filter = (plugin_dat_filter_fn) (intptr_t)
		acl_dlsym(handle, "http_respond_dat_filter");
	plugin.data_free = (plugin_dat_free_fn) (intptr_t)
		acl_dlsym(handle, "http_respond_dat_free");
	if (plugin.data_filter) {
		http_plugin = (HTTP_PLUGIN*) acl_mycalloc(1, sizeof(HTTP_PLUGIN));
		http_plugin->data_filter = plugin.data_filter;
		http_plugin->data_free = plugin.data_free;
		acl_fifo_push(&__respond_dat_plugins, http_plugin);
	}
}

void http_plugin_load_all(ACL_DLL_ENV *dll_env, const char *dlnames, const char *plugin_cfgdir)
{
	ACL_ARGV *argv;
	ACL_ITER iter;

	acl_fifo_init(&__request_plugins);
	acl_fifo_init(&__request_dat_plugins);
	acl_fifo_init(&__respond_plugins);
	acl_fifo_init(&__respond_dat_plugins);
	if (dlnames == NULL || *dlnames == 0)
		return;

	argv = acl_argv_split(dlnames, " \t,;");
	acl_foreach(iter, argv) {
		const char *dlname = (const char*) iter.data;

		http_plugin_load(dll_env, dlname, plugin_cfgdir);
	}
	acl_argv_free(argv);
}

void http_plugin_unload_all()
{
	ACL_ITER iter;

	if (__handles == NULL)
		return;

	acl_foreach(iter, __handles) {
		void *handle = (void*) iter.data;
		acl_dlclose(handle);
	}
}

void http_plugin_set_callback(HTTP_SERVICE *service)
{
	ACL_ITER iter;
	HTTP_PLUGIN *http_plugin, *plugin;

	/* 初始化过滤器队列集合 */
	acl_fifo_init(&service->request_plugins);
	acl_fifo_init(&service->respond_plugins);
	acl_fifo_init(&service->request_dat_plugins);
	acl_fifo_init(&service->respond_dat_plugins);

	acl_foreach(iter, &__request_plugins) {
		plugin = (HTTP_PLUGIN*) iter.data;
		http_plugin = (HTTP_PLUGIN*) acl_mycalloc(1, sizeof(HTTP_PLUGIN));
		http_plugin->filter.request = plugin->filter.request;
		http_plugin->forward.request = plugin->forward.request;
		acl_fifo_push(&service->request_plugins, http_plugin);
	}

	acl_foreach(iter, &__request_dat_plugins) {
		plugin = (HTTP_PLUGIN*) iter.data;
		http_plugin = (HTTP_PLUGIN*) acl_mycalloc(1, sizeof(HTTP_PLUGIN));
		http_plugin->data_filter = plugin->data_filter;
		http_plugin->data_free = plugin->data_free;
		acl_fifo_push(&service->request_dat_plugins, http_plugin);
	}

	acl_foreach(iter, &__respond_plugins) {
		plugin = (HTTP_PLUGIN*) iter.data;
		http_plugin = (HTTP_PLUGIN*) acl_mycalloc(1, sizeof(HTTP_PLUGIN));
		http_plugin->filter.respond = plugin->filter.respond;
		http_plugin->forward.respond = plugin->forward.respond;
		acl_fifo_push(&service->respond_plugins, http_plugin);
	}

	acl_foreach(iter, &__respond_dat_plugins) {
		plugin = (HTTP_PLUGIN*) iter.data;
		http_plugin = (HTTP_PLUGIN*) acl_mycalloc(1, sizeof(HTTP_PLUGIN));
		http_plugin->data_filter = plugin->data_filter;
		http_plugin->data_free = plugin->data_free;
		acl_fifo_push(&service->respond_dat_plugins, http_plugin);
	}
}
