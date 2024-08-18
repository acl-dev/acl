#pragma once
#include <list>

class user_client
{
public:
	user_client(ACL_EVENT*event, ACL_VSTREAM* conn)
		: event_(event), conn_(conn), busy_(false) {}
	~user_client(void)
	{
		for (std::list<acl::string*>::iterator it = messages_.begin();
			it != messages_.end(); ++it)
		{
			delete *it;
		}
	}

	ACL_EVENT* get_event(void) const
	{
		return event_;
	}

	ACL_VSTREAM* get_stream(void) const
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

	bool is_busy(void) const
	{
		return busy_;
	}

	void set_busy(bool yes)
	{
		busy_ = yes;
	}

private:
	ACL_EVENT* event_;
	ACL_VSTREAM* conn_;
	acl::string name_;
	std::list<acl::string*> messages_;
	bool busy_;
};
