#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/aio_timer_callback.hpp"
#include "aio_timer_delay_free.hpp"
#include "acl_cpp/stream/aio_handle.hpp"

namespace acl
{

aio_handle::aio_handle(aio_handle_type engine_type /* = ENGINE_SELECT */,
	unsigned int nMsg /* = 0 */)
: stop_(false)
, nstream_(0)
, engine_type_(engine_type)
{
	int   event_type;

	if (engine_type == ENGINE_SELECT)
		event_type = ACL_EVENT_SELECT;
	else if (engine_type == ENGINE_POLL)
		event_type = ACL_EVENT_POLL;
	else if (engine_type == ENGINE_KERNEL)
		event_type = ACL_EVENT_KERNEL;
#ifdef WIN32
	else if (engine_type == ENGINE_WINMSG)
		event_type = ACL_EVENT_WMSG;
#endif
	else
	{
		event_type = ENGINE_SELECT;  // xxx: just avoid gcc warning
		acl_assert(0);
	}

	aio_ = acl_aio_create2(event_type, nMsg);
	inner_alloc_ = true;

	// �����ӳٹرյĶ�ʱ������
	delay_free_timer_ = NEW aio_timer_delay_free(*this);

	// ���첽�������������������������ӳ��ͷŶ�ʱ
	// �����Է�ֹ�첽�����Զ����ٸö�ʱ��
	delay_free_timer_->set_locked();
}

aio_handle::aio_handle(ACL_AIO* aio)
	: aio_(aio)
	, stop_(false)
	, nstream_(0)
{
	acl_assert(aio_);
	int event_type = acl_aio_event_mode(aio);

	if (event_type == ACL_EVENT_SELECT)
		engine_type_ = ENGINE_SELECT;
	else if (event_type == ACL_EVENT_POLL)
		engine_type_ = ENGINE_POLL;
	else if (event_type == ACL_EVENT_KERNEL)
		engine_type_ = ENGINE_KERNEL;
	else if (event_type == ACL_EVENT_WMSG)
		engine_type_ = ENGINE_WINMSG;
	else
		acl_assert(0);

	inner_alloc_ = false;

	// �����ӳٹرյĶ�ʱ������
	delay_free_timer_ = NEW aio_timer_delay_free(*this);
	// ���첽�������������������������ӳ��ͷŶ�ʱ
	// �����Է�ֹ�첽�����Զ����ٸö�ʱ��
	delay_free_timer_->set_locked();
}

aio_handle::~aio_handle()
{
	if (inner_alloc_)
		acl_aio_free(aio_);

	delay_free_timer_->unset_locked();
	delete delay_free_timer_;
}

void aio_handle::keep_read(bool onoff)
{
	acl_aio_set_keep_read(aio_, onoff ? 1 : 0);
}

bool aio_handle::keep_read() const
{
	return acl_aio_get_keep_read(aio_) == 0 ? false : true;
}

acl_int64 aio_handle::set_timer(aio_timer_callback* callback,
	acl_int64 delay, unsigned int id /* = 0 */)
{
	acl_assert(aio_);
	callback->handle_ = this;
	delay = callback->set_task(id, delay);
	return acl_aio_request_timer(aio_,
		(void (*)(int, ACL_EVENT*, void*)) on_timer_callback,
		callback, delay < 0 ? 0 : delay,
		callback->keep_ ? 1 : 0);
}

void aio_handle::on_timer_callback(int, ACL_EVENT*,
	aio_timer_callback *callback)
{
	acl_assert(callback);
	acl_assert(callback->handle_);
	aio_handle* handle = callback->handle_;

	// ������ʱ���е����ж�ʱ����
	acl_int64 next_delay = callback->trigger();

	// �����ʱ���е�����Ϊ�ջ�δ���ö�ʱ�����ظ�ʹ�ã���ɾ����ʱ��

	if (callback->empty())
	{
		logger("timer empty, delete it");
		handle->del_timer(callback);
		return;
	}

	if (!callback->keep_timer())
	{
		logger("timer no keep delete it");
		handle->del_timer(callback);
		return;
	}

	// ��������ظ�ʹ�ö�ʱ���Ҷ�ʱ���е�����ǿգ����ٴ����øö�ʱ��

	//  ��Ҫ���ö�ʱ���ĵ���ʱ���
	acl_aio_request_timer(handle->aio_,
		(void (*)(int, ACL_EVENT*, void*)) on_timer_callback,
		callback, next_delay < 0 ? 0 : next_delay,
		callback->keep_timer() ? 1 : 0);
}

acl_int64 aio_handle::del_timer(aio_timer_callback* callback, unsigned int id)
{
	acl_assert(aio_);
	acl_int64 next_delay = callback->del_task(id);

	// �����ʱ���е�����Ϊ�գ���ɾ���ö�ʱ��
	if (callback->empty())
		return del_timer(callback);

	return acl_aio_request_timer(callback->handle_->aio_,
		(void (*)(int, ACL_EVENT*, void*)) on_timer_callback,
		callback, next_delay < 0 ? 0 : next_delay,
		callback->keep_timer() ? 1 : 0);
}

acl_int64 aio_handle::del_timer(aio_timer_callback* callback)
{
	acl_assert(aio_);
	acl_int64 next_delay = acl_aio_cancel_timer(aio_,
		(void (*)(int, ACL_EVENT*, void*)) on_timer_callback,
		callback);
	callback->clear();
	destroy_timer(callback);

	return next_delay;
}

void aio_handle::delay_free(aio_delay_free* callback)
{
	// ����ӳ��ͷŶ�ʱ���Ķ�ʱ����
	if (delay_free_timer_->add(callback))
		set_timer(delay_free_timer_, 100000, 0);
}

void aio_handle::destroy_timer(aio_timer_callback* callback)
{
	delay_free_timer_->del(callback);

	// ����ö�ʱ���Ѿ��� trigger �б���������ֻ��Ҫ
	// �����ڽ�������Ҫ�������ٵı�ʶ���ɣ����䱾��
	// ��������
	if (callback->locked())
		callback->destroy_on_unlock_ = true;

	// ֱ������δ�����Ķ�ʱ��
	else
		callback->destroy();
}

ACL_AIO* aio_handle::get_handle() const
{
	return aio_;
}

aio_handle_type aio_handle::get_engine_type() const
{
	return engine_type_;
}

bool aio_handle::check()
{
	acl_aio_loop(aio_);
	if (stop_)
		return false;
	return true;
}

void aio_handle::stop()
{
	stop_ = true;
}

void aio_handle::reset()
{
	stop_ = false;
}

void aio_handle::increase()
{
	nstream_++;
	on_increase();
}

void aio_handle::decrease()
{
	nstream_--;
	acl_assert(nstream_ >= 0);
	on_decrease();
}

int aio_handle::length() const
{
	return nstream_;
}

}  // namespace acl
