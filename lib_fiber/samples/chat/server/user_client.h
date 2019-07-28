#pragma once
#include <list>

enum
{
	MT_MSG,
	MT_LOGOUT,
	MT_KICK,
};

class user_client
{
public:
#ifdef USE_CHAN
	user_client(acl::socket_stream& conn) : conn_(conn) {}
#else
	user_client(acl::socket_stream& conn) : conn_(conn), sem_msg_(1) {}
#endif
	~user_client(void)
	{
		for (std::list<acl::string*>::iterator it = messages_.begin();
			it != messages_.end(); ++it)
		{
			delete *it;
		}
	}

	acl::socket_stream& get_stream(void) const
	{
		return conn_;
	}

	bool already_login(void) const
	{
		return !name_.empty();
	}

	bool empty(void) const
	{
		return messages_.empty();
	}

	acl::string* pop(void)
	{
		if (messages_.empty())
			return NULL;
		acl::string* msg = messages_.front();
		messages_.pop_front();
		return msg;
	}

	void push(const char* msg)
	{
		acl::string* buf = new acl::string(msg);
		(*buf) << "\r\n";
		messages_.push_back(buf);
	}

	void set_name(const char* name)
	{
		name_ = name;
	}

	const char* get_name(void) const
	{
		return name_.c_str();
	}

	void wait(int& mtype)
	{
#ifdef USE_CHAN
		chan_msg_.pop(mtype);
#else
		(void) mtype;
		sem_msg_.wait();
#endif
	}

	void notify(int mtype)
	{
#ifdef USE_CHAN
		chan_msg_.put(mtype);
#else
		if (mtype == MT_LOGOUT)
			exiting_ = true;
		(void) sem_msg_.post();
#endif
	}

	void kill_reader(void)
	{
		if (fiber_reader_)
		{
			exiting_ = true;
			acl_fiber_kill(fiber_reader_);
		}
	}

	bool exiting(void) const
	{
		return exiting_;
	}

	void set_exiting(void)
	{
		exiting_ = true;
	}

	void set_reading(bool yes)
	{
		reading_ = yes;
	}

	bool is_reading(void) const
	{
		return reading_;
	}

	void set_waiting(bool yes)
	{
		waiting_ = yes;
	}

	bool is_waiting(void) const
	{
		return waiting_;
	}

	void wait_exit(void)
	{
		int mtype;
		chan_exit_.pop(mtype);
	}

	void notify_exit(void)
	{
		int mtype = MT_LOGOUT;
		chan_exit_.put(mtype);
	}

	void set_reader(void)
	{
		fiber_reader_ = acl_fiber_running();
	}

	void set_waiter(void)
	{
		fiber_waiter_ = acl_fiber_running();
	}

private:
	acl::socket_stream& conn_;
#ifdef	USE_CHAN
	acl::channel<int> chan_msg_;
#else
	acl::fiber_sem	  sem_msg_;
#endif
	acl::channel<int> chan_exit_;
	acl::string name_;
	std::list<acl::string*> messages_;
	bool exiting_ = false;
	bool reading_ = false;
	bool waiting_ = false;
	ACL_FIBER* fiber_reader_ = NULL;
	ACL_FIBER* fiber_waiter_ = NULL;
};
