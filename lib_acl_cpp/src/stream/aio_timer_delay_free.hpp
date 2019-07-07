#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <set>
#include "acl_cpp/stream/aio_timer_callback.hpp"

namespace acl
{

class aio_handle;
class aio_delay_free;

class aio_timer_delay_free : public aio_timer_callback
{
public:
	aio_timer_delay_free(aio_handle& handle);
	~aio_timer_delay_free(void);

	virtual void timer_callback(unsigned int id);
	bool add(aio_delay_free* callback);
	bool del(aio_delay_free* callback);

private:
	aio_handle& handle_;
	std::set<aio_delay_free*> gc_set_;
};

} // namespace acl
