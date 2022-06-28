#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/connpool/connect_monitor.hpp"
#include "acl_cpp/connpool/check_client.hpp"
#endif
#include "check_rpc.hpp"

namespace acl
{

check_rpc::check_rpc(connect_monitor& monitor, check_client& checker)
: monitor_(monitor)
, checker_(checker)
{
}

check_rpc::~check_rpc(void)
{
}

void check_rpc::rpc_run(void)
{
	// 打开阻塞流对象
	socket_stream stream;

	// 必须用 get_vstream() 获得的 ACL_VSTREAM 流对象做参数
	// 来打开 stream 对象，因为在 acl_cpp 和 acl 中的阻塞流
	// 和非阻塞流最终都是基于 ACL_VSTREAM，而 ACL_VSTREAM 流
	// 内部维护着了一个读/写缓冲区，所以在长连接的数据处理中，
	// 必须每次将 ACL_VSTREAM 做为内部流的缓冲流来对待
	ACL_VSTREAM* vstream = checker_.get_conn().get_vstream();
	ACL_VSTREAM_SET_RWTIMO(vstream, 10);
	(void) stream.open(vstream);
	// 设置为阻塞模式
	stream.set_tcp_non_blocking(false);

	// 调用同步检测过程
	monitor_.sio_check(checker_, stream);

	// 设置为非阻塞模式
	stream.set_tcp_non_blocking(true);

	// 将 ACL_VSTREAM 与阻塞流对象解绑定，这样才能保证当释放阻塞流对象时
	// 不会关闭与请求者的连接，因为该连接本身是属于非阻塞流对象的，需要采
	// 用异步流关闭方式进行关闭
	stream.unbind();
}

void check_rpc::rpc_onover(void)
{
	// 取消该检测对象的阻塞状态
	checker_.set_blocked(false);

	// 异步关闭检测对象
	checker_.close();

	// 删除动态 rpc 请求对象
	delete this;
}

} // namespace acl
