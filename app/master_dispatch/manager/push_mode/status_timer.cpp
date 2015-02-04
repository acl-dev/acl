#include "stdafx.h"
#include "push_mode/status_manager.h"
#include "status_timer.h"

status_timer::status_timer()
{
}

status_timer::~status_timer()
{
}

void status_timer::destroy()
{
	delete this;
}

void status_timer::timer_callback(unsigned int)
{
	int ret = status_manager::get_instance().check_timeout();
	if (ret > 0)
		logger("delete %d server's status", ret);
}
