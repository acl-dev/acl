#include "lib_acl.h"
#include "service.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

void client_entry_free(CLIENT_ENTRY *entry)
{
	acl_myfree(entry);
}

/* 当客户端异步流被关闭时的回调函数 */

static void onclose_client_stream(ACL_VSTREAM *stream, void *arg)
{
	const char *myname = "onclose_client_stream";
	CLIENT_ENTRY *entry = (CLIENT_ENTRY*) arg;

	acl_msg_info("%s(%d), fd: %d, nrefer: %d, nrefer: %d, addr: %lx, %lx, domain(%s)",
		__FUNCTION__, __LINE__, ACL_VSTREAM_SOCK(stream),
		stream->nrefer, entry->nrefer,
		(long) entry, (long) entry->client, entry->domain_key);

	if (stream->nrefer != 0)
		acl_msg_fatal("%s: stream->nrefer(%d) != 0", myname, stream->nrefer);

	/* 需要提前设置为空, 以防在调用 acl_aio_iocp_close 后又要操作该流指针 */
	entry->client = NULL;

	if (entry->server)
		acl_aio_iocp_close(entry->server);

	/* 需要放在 acl_aio_iocp_lose() 后面，以防止 entry 对象提前释放 */
	entry->nrefer--;
	if (entry->nrefer > 0)
		return;
	if (entry->server != NULL)
		acl_msg_fatal("%s(%d): server not null", myname, __LINE__);

	entry->free_fn(entry);
}

CLIENT_ENTRY *client_entry_new(SERVICE *service, size_t size, ACL_ASTREAM *client)
{
	const char *myname = "client_entry_new";
	CLIENT_ENTRY *entry;
	ACL_VSTREAM *stream;

	if (size < sizeof(CLIENT_ENTRY))
		acl_msg_fatal("%s(%d): size(%d) invalid", myname, __LINE__, size);

	entry = (CLIENT_ENTRY* ) acl_mycalloc(1, size);
	entry->service = service;

	entry->client = client;
	entry->nrefer++;

	stream = acl_aio_vstream(client);
	acl_vstream_add_close_handle(stream, onclose_client_stream, entry);

	return (entry);
}

/* 当服务端异步流被关闭时的回调函数 */

static void onclose_server_stream(ACL_VSTREAM *stream, void *arg)
{
	const char *myname = "onclose_server_stream";
	CLIENT_ENTRY *entry = (CLIENT_ENTRY*) arg;

	acl_msg_info("%s(%d), nrefer: %d", __FUNCTION__, __LINE__, stream->nrefer);

	if (stream->nrefer != 0)
		acl_msg_fatal("%s: stream->nrefer(%d) != 0", myname, stream->nrefer);

	/* 需要提前设置为空, 以防在调用 acl_aio_iocp_close 后又要操作该流指针 */
	entry->server = NULL;

	if (entry->client) {
		acl_aio_iocp_close(entry->client);
	}

	/* 需要放在 acl_aio_iocp_lose() 后面，以防止 entry 对象提前释放 */
	entry->nrefer--;
	if (entry->nrefer > 0)
		return;
	if (entry->client != NULL)
		acl_msg_fatal("%s(%d): client not null", myname, __LINE__);

	entry->free_fn(entry);
}

void client_entry_set_server(CLIENT_ENTRY *entry, ACL_ASTREAM *server)
{
	const char *myname = "client_entry_set_server";
	ACL_VSTREAM *stream;

	if (entry->server != NULL)
		acl_msg_fatal("%s(%d): entry->server not null", myname, __LINE__);
	entry->server = server;
	entry->nrefer++;
	stream = acl_aio_vstream(server);
	acl_vstream_add_close_handle(stream, onclose_server_stream, entry);
}

int client_entry_detach(CLIENT_ENTRY *entry, ACL_VSTREAM *stream)
{
	return (client_entry_detach3(entry, stream, 1));
}

int client_entry_detach3(CLIENT_ENTRY *entry, ACL_VSTREAM *stream, int auto_free)
{
	const char *myname = "client_entry_detach";

	/* 必须取消在关闭流时的回调函数 */
	if (entry->client && acl_aio_vstream(entry->client) == stream) {
		/* 删除流的关闭回调函数句柄，这样仅清除与该应用相关的删除回调 */
		acl_vstream_delete_close_handle(stream, onclose_client_stream, entry);
		entry->client = NULL;
		entry->nrefer--;
	} else if (entry->server && acl_aio_vstream(entry->server) == stream) {
		/* 删除流的关闭回调函数句柄，这样仅清除与该应用相关的删除回调 */
		acl_vstream_delete_close_handle(stream, onclose_server_stream, entry);
		entry->server = NULL;
		entry->nrefer--;
	} else {
		acl_msg_fatal("%s(%d): unknown stream", myname, __LINE__);
	}

	/* 如果 entry 引用对象计数为0则需要释放掉该对象 */
	if (entry->nrefer == 0) {
		if (auto_free)
			entry->free_fn(entry);
		/* 告诉调用者， entry 资源已经被释放，不能再使用 */
		return (1);
	} else if (entry->client == NULL && entry->server == NULL) {
		acl_msg_warn("%s(%d): nrefer=%d, client %s, server %s\n",
			myname, __LINE__, entry->nrefer,
			entry->client ? "not null" : "null",
			entry->server ? "not null" : "null");
	}

	return (0);
}
