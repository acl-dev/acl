#pragma once

class status_timer : public acl::event_timer
{
public:
	status_timer();

	/**
	 * 定时器被销毁时的回调函数 
	 */
	virtual void destroy();

protected:
	// 定时器时间到到达时的回调函数 
	virtual void timer_callback(unsigned int id);

private:
	~status_timer();
};
