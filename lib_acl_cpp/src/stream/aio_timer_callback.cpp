#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/aio_timer_callback.hpp"
#include <set>

namespace acl
{

//////////////////////////////////////////////////////////////////////////

class aio_timer_task
{
public:
	aio_timer_task() {}
	~aio_timer_task() {}

private:
	friend class aio_timer_callback;

	unsigned int id;
	acl_int64 delay;
	acl_int64 when;
};

//////////////////////////////////////////////////////////////////////////

aio_timer_callback::aio_timer_callback(bool keep /* = false */)
{
	keep_ = keep;
	length_ = 0;
}

aio_timer_callback::~aio_timer_callback(void)
{
	// ������ڴ�����ʱ���Ļص��������������̱�������ᷢ����������
	if (locked())
	{
		logger_error("In trigger proccess, you delete me now!");
		acl_assert(0);
	}

	clear();
}

int aio_timer_callback::clear()
{
	int  n = 0;
	std::list<aio_timer_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it)
	{
		delete (*it);
		n++;
	}
	tasks_.clear();
	length_ = 0;
	return n;
}

bool aio_timer_callback::empty() const
{
	return tasks_.empty();
}

size_t aio_timer_callback::length() const
{
	return length_;
}

void aio_timer_callback::keep_timer(bool on)
{
	keep_ = on;
}

bool aio_timer_callback::keep_timer(void) const
{
	return keep_;
}

void aio_timer_callback::set_time(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	present_ = ((acl_int64) now.tv_sec) * 1000000
		+ ((acl_int64) now.tv_usec);
}

#define TIMER_EMPTY		-1

acl_int64 aio_timer_callback::del_task(unsigned int id)
{
	bool ok = false;
	std::list<aio_timer_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it)
	{
		if ((*it)->id == id)
		{
			delete (*it);
			tasks_.erase(it);
			length_--;
			ok = true;
			break;
		}
	}

	if (!ok)
		logger_warn("timer id: %u not found", id);

	if (tasks_.empty())
		return TIMER_EMPTY;

	set_time();

	acl_int64 delay = tasks_.front()->when - present_;
	return delay < 0 ? 0 : delay;
}

acl_int64 aio_timer_callback::set_task(unsigned int id, acl_int64 delay)
{
	aio_timer_task* task = NULL;
	std::list<aio_timer_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it)
	{
		if ((*it)->id == id)
		{
			task = (*it);
			tasks_.erase(it);
			length_--;
			break;
		}
	}

	if (task == NULL)
	{
		task = NEW aio_timer_task();
		task->delay = delay;
		task->id = id;
	}
	else
		task->delay = delay;

	return set_task(task);
}

acl_int64 aio_timer_callback::set_task(aio_timer_task* task)
{
	set_time();
	task->when = present_ + task->delay;

	std::list<aio_timer_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it)
	{
		if (task->when < (*it)->when)
		{
			tasks_.insert(it, task);
			break;
		}
	}

	if (it == tasks_.end())
		tasks_.push_back(task);

	length_++;

	aio_timer_task* first = tasks_.front();
	acl_int64 delay = first->when - present_;
	return delay < 0 ? 0 : delay;
}

acl_int64 aio_timer_callback::trigger(void)
{
	// sanity check
	if (tasks_.empty())
		return TIMER_EMPTY;

	acl_assert(length_ > 0);

	set_time();

	std::list<aio_timer_task*>::iterator it, next;
	std::list<aio_timer_task*> tasks;
	aio_timer_task* task;

	// �Ӷ�ʱ����ȡ������Ķ�ʱ����
	for (it = tasks_.begin(); it != tasks_.end(); it = next)
	{
		if ((*it)->when > present_)
			break;
		next = it;
		++next;
		task = *it;
		tasks_.erase(it);
		length_--;
		tasks.push_back(task);
	}

	// �п�����Щ����Ķ�ʱ�����Ѿ����û���ǰɾ����
	if (tasks.empty())
	{
		acl_assert(!tasks_.empty());

		aio_timer_task* first = tasks_.front();
		acl_int64 delay = first->when - present_;
		return delay < 0 ? 0 : delay;
	}

	// ������Ķ�ʱ�������·Ż�����ʱ���������б��У�
	// ����ʼ�������еĵ���Ķ�ʱ����

	// ���������ô�������æ״̬���Է�ֹ�����ڻص�����
	// �е����˸���������������
	set_locked();

	// ���ý��������ٱ�־Ϊ false����Ϊ��ǰ�ö�ʱ������
	// ����״̬�����������������ֱ��������ʱ���ٱ���
	// ���󣬵�����������ñ�ʶ����Ϊ true���������
	// Ӧ���Զ�����
	destroy_on_unlock_ = false;

	for (it = tasks.begin(); it != tasks.end(); ++it)
	{
		set_task(*it);
		timer_callback((*it)->id);
	}

	tasks.clear();

	// ����֮��Ĳ����б����������������
	unset_locked();

	// �����п��ܻ��� timer_callback ��ɾ�������еĶ�ʱ����
	if (tasks_.empty())
		return TIMER_EMPTY;

	aio_timer_task* first = tasks_.front();
	acl_int64 delay = first->when - present_;

	// ����ڼ����ڼ��ⲿ����Ҫ���ͷŸö������ڴ˴��ͷ�
	if (destroy_on_unlock_)
	{
		destroy();
		return TIMER_EMPTY;
	}
	return delay < 0 ? 0 : delay;
}

} // namespace acl
