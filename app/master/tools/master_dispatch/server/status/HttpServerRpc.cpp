#include "stdafx.h"
#include "status/StatusServlet.h"
#include "status/HttpServerRpc.h"

HttpServerRpc::HttpServerRpc(acl::aio_socket_stream* client)
: client_(client)
, keep_alive_(false)
{
}

HttpServerRpc::~HttpServerRpc()
{
}

// 在子线程中处理
void HttpServerRpc::rpc_run()
{
	// 打开阻塞流对象
	acl::socket_stream stream;

	// 必须用 get_vstream() 获得的 ACL_VSTREAM 流对象做参数
	// 来打开 stream 对象，因为在 acl_cpp 和 acl 中的阻塞流
	// 和非阻塞流最终都是基于 ACL_VSTREAM，而 ACL_VSTREAM 流
	// 内部维护着了一个读/写缓冲区，所以在长连接的数据处理中，
	// 必须每次将 ACL_VSTREAM 做为内部流的缓冲流来对待
	ACL_VSTREAM* vstream = client_->get_vstream();
	ACL_VSTREAM_SET_RWTIMO(vstream, var_cfg_rw_timeout);

	(void) stream.open(vstream);

	// 开始处理该 HTTP 请求
	handle_http(stream);

	// 将 ACL_VSTREAM 与阻塞流对象解绑定，这样才能保证当释放阻塞流对象时
	// 不会关闭与请求者的连接，因为该连接本身是属于非阻塞流对象的，需要采
	// 用异步流关闭方式进行关闭
	stream.unbind();
}

void HttpServerRpc::handle_http(acl::socket_stream& stream)
{
	StatusServlet servlet;
	acl::memcache_session session(var_cfg_session_addr);

	servlet.setLocalCharset("gb2312");

	// 是否允许与客户端之间保持长连接
	if (servlet.doRun(session, &stream) == true && servlet.keep_alive())
		keep_alive_ = true;
}

/////////////////////////////////////////////////////////////////////////////

// 在主线程中处理
void HttpServerRpc::rpc_onover()
{
	if (keep_alive_)
		client_->read_wait(var_cfg_rw_timeout);
	else
		client_->close();
}
