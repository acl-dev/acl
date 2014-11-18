#include "stdafx.h"
#include "http_client.h"
#include "master_service.h"

////////////////////////////////////////////////////////////////////////////////
// 配置内容项

int  var_cfg_preread;
acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "preread", 1, &var_cfg_preread },

	{ 0, 0, 0 }
};

int  var_cfg_buf_size;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "buf_size", 1024, &var_cfg_buf_size, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

////////////////////////////////////////////////////////////////////////////////

master_service::master_service()
{
}

master_service::~master_service()
{
}

bool master_service::on_accept(acl::aio_socket_stream* client)
{
	//acl_tcp_nodelay(client->get_socket(), 1);

	// 如果允许在主线程中预读，则设置流的预读标志位
	if (var_cfg_preread)
	{
		ACL_VSTREAM* vstream = client->get_vstream();
		vstream->flag |= ACL_VSTREAM_FLAG_PREREAD;
	}

	// 创建异步客户端流的回调对象并与该异步流进行绑定
	http_client* callback = new http_client(client, var_cfg_buf_size);

	// 注册异步流的读回调过程
	client->add_read_callback(callback);

	// 注册异步流的写回调过程
	client->add_write_callback(callback);

	// 注册异步流的关闭回调过程
	client->add_close_callback(callback);

	// 注册异步流的超时回调过程
	client->add_timeout_callback(callback);

	client->keep_read(true);

	// 监控异步流是否可读
	client->read_wait(0);

	return true;
}

void master_service::proc_on_init()
{
}

void master_service::proc_on_exit()
{
}
