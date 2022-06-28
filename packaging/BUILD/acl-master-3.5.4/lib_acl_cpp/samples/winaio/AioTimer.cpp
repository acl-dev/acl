#include "StdAfx.h"
#include <iostream>
#include "acl_cpp/stream/aio_handle.hpp"
#include "winaioDlg.h"
#include "AioTimer.hpp"
//#include <new>

//using namespace acl;

CAioTimer::CAioTimer(acl::aio_handle* handle, int id, bool keep)
: acl::aio_timer_callback(keep)
, handle_(handle)
, id_(id)
, inited_(false)
{

}

CAioTimer::~CAioTimer()
{
	std::cout << "timer deleted" << std::endl;
}

void CAioTimer::timer_callback(unsigned int id)
{
	const char* s = keep_timer() ? "keep timer" : "cancel timer";
	std::cout << "timer " << id_ << ':' << id << " callback! " << s << std::endl;
	if (inited_)
		return;
	inited_ = true;
#if 0
	handle_->del_timer(this);
	handle_->delay_free(this);
#elif 1
	set_task(1, 5000000);
	set_task(2, 4000000);
	set_task(3, 3000000);
	set_task(4, 2000000);
	set_task(5, 1000000);
#endif
}

void CAioTimer::destroy(void)
{
	delete this;
}