#pragma once

class rpc_timer : public acl::aio_timer_callback
{
public:
	rpc_timer(acl::aio_handle& handle);

	void start(int delay);
	void stop();
private:
	~rpc_timer();

	acl::aio_handle& handle_;

	// 基类纯虚函数
	virtual void timer_callback(unsigned int id);
	virtual void destroy(void);
};
