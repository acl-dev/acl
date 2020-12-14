#include "stdafx.h"
#include "rpc_stats.h"
#include "http_rpc.h"

http_rpc::http_rpc(acl::aio_socket_stream* client, unsigned buf_size)
: client_(client)
, buf_size_(buf_size)
{
	res_buf_ = (char*) acl_mymalloc(buf_size + 1);
	unsigned i;
	for (i = 0; i < buf_size; i++)
		res_buf_[i] = 'x';
	res_buf_[i] = 0;
}

http_rpc::~http_rpc()
{
	//logger("rpc_request destroyed!");
	acl_myfree(res_buf_);
}

// 调用 service_.rpc_fork 后，由 RPC 框架在子线程中调用本函数
// 来处理本地其它模块发来的请求信息
void http_rpc::rpc_run()
{
	// 打开阻塞流对象
	acl::socket_stream stream;

	// 必须用 get_vstream() 获得的 ACL_VSTREAM 流对象做参数
	// 来打开 stream 对象，因为在 acl_cpp 和 acl 中的阻塞流
	// 和非阻塞流最终都是基于 ACL_VSTREAM，而 ACL_VSTREAM 流
	// 内部维护着了一个读/写缓冲区，所以在长连接的数据处理中，
	// 必须每次将 ACL_VSTREAM 做为内部流的缓冲流来对待
	ACL_VSTREAM* vstream = client_->get_vstream();
	ACL_VSTREAM_SET_RWTIMO(vstream, 10);
	(void) stream.open(vstream);
	// 设置为阻塞模式
	stream.set_tcp_non_blocking(false);

	rpc_req_add();

	// 开始处理该 HTTP 请求
	handle_conn(&stream);

	rpc_req_del();

	// 设置为非阻塞模式
	stream.set_tcp_non_blocking(true);

	// 将 ACL_VSTREAM 与阻塞流对象解绑定，这样才能保证当释放阻塞流对象时
	// 不会关闭与请求者的连接，因为该连接本身是属于非阻塞流对象的，需要采
	// 用异步流关闭方式进行关闭
	stream.unbind();
}

void http_rpc::handle_conn(acl::socket_stream* stream)
{
	// HTTP 响应对象构造
	acl::http_response res(stream);
	// 响应数据体为 xml 格式
	res.response_header().set_content_type("text/html");

	// 读 HTTP 请求头
	if (res.read_header() == false)
	{
		keep_alive_ = false;
		return;
	}

	acl::string buf;
	// 读 HTTP 请求体数据
	if (res.get_body(buf) == false)
	{
		keep_alive_ = false;
		return;
	}

	acl::http_client* client = res.get_client();

	// 判断客户端是否希望保持长连接
	keep_alive_ = client->keep_alive();

	// 返回数据给客户端

	res.response_header()
		.set_status(200)
		.set_keep_alive(keep_alive_)
		.set_content_length(buf_size_);;
	res.response(res_buf_, buf_size_);
}

void http_rpc::rpc_onover()
{
	// 减少 rpc 计数
	rpc_del();

	if (keep_alive_)
	{
		rpc_read_wait_add();

		// 监控异步流是否可读
		client_->read_wait(10);
	}
	else
		// 关闭异步流对象
		client_->close();
}
