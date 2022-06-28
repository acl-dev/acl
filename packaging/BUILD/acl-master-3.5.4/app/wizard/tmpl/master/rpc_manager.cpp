#include "stdafx.h"
#include "rpc_manager.h"

rpc_manager::rpc_manager(void)
: handle_(NULL)
{

}

rpc_manager::~rpc_manager(void)
{
	delete service_;
	if (handle_ != NULL) {
		handle_->check();
	}
	logger("rpc service destroy ok!");
}

void rpc_manager::init(acl::aio_handle& handle, int max_threads /* = 10 */,
	const char* addr /* = NULL */)
{
	handle_ = &handle;

	// 创建 rpc 服务对象
	service_ = new acl::rpc_service(max_threads);

	// 打开消息服务
	if (!service_->open(handle_, addr && *addr ? addr : NULL)) {
		logger_fatal("open service error: %s", acl::last_serror());
	} else {
		logger("open service ok, listen: %s", service_->get_addr());
	}
}

void rpc_manager::fork(acl::rpc_request* req)
{
	service_->rpc_fork(req);
}
