#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/event/event_timer.hpp"
#include "acl_cpp/master/master_base.hpp"
#endif

namespace acl
{

master_base::master_base()
{
	daemon_mode_ = false;
	proc_inited_ = false;
	event_ = NULL;
}

master_base::~master_base()
{

}

void master_base::set_cfg_bool(master_bool_tbl* table)
{
	if (table == NULL)
		return;
	conf_.set_cfg_bool(table);
}

void master_base::set_cfg_int(master_int_tbl* table)
{
	if (table == NULL)
		return;
	conf_.set_cfg_int(table);
}

void master_base::set_cfg_int64(master_int64_tbl* table)
{
	if (table == NULL)
		return;
	conf_.set_cfg_int64(table);
}

void master_base::set_cfg_str(master_str_tbl* table)
{
	if (table == NULL)
		return;
	conf_.set_cfg_str(table);
}

bool master_base::daemon_mode() const
{
	return daemon_mode_;
}

static void timer_callback(int, ACL_EVENT* event, void* ctx)
{
	event_timer* timer = (event_timer*) ctx;

	// 触发定时器中的所有定时任务
	acl_int64 next_delay = timer->trigger();

	// 如果定时器中的任务为空或未设置定时器的重复使用，则删除定时器
	if (timer->empty() || !timer->keep_timer())
	{
		// 删除定时器
		acl_event_cancel_timer(event, timer_callback, timer);
		timer->destroy();
		return;
	}

	// 如果允许重复使用定时器且定时器中的任务非空，则再次设置该定时器

	//  需要重置定时器的到达时间截
	acl_event_request_timer(event, timer_callback, timer,
		next_delay < 0 ? 0 : next_delay, timer->keep_timer() ? 1 : 0);
}

void master_base::set_event(ACL_EVENT* event)
{
	event_ = event;
}

bool master_base::proc_set_timer(event_timer* timer)
{
	if (event_ == NULL)
	{
		logger_warn("event NULL!");
		return false;
	}
	else
	{
		acl_event_request_timer(event_, timer_callback, timer,
			timer->min_delay(), timer->keep_timer() ? 1 : 0);
		return true;
	}
}

void master_base::proc_del_timer(event_timer* timer)
{
	if (event_ == NULL)
		logger_warn("event NULL!");
	else
		acl_event_cancel_timer(event_, timer_callback, timer);
}

}  // namespace acl
