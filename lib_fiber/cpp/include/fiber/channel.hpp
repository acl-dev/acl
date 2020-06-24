#pragma once

#include "fiber_cpp_define.hpp"

struct ACL_CHANNEL;

namespace acl {

ACL_CHANNEL *channel_create(int elemsize, int bufsize);
void channel_free(ACL_CHANNEL *c);
int channel_send(ACL_CHANNEL *c, void *v);
int channel_recv(ACL_CHANNEL *c, void *v);

template <typename T>
class channel
{
public:
	channel(void)
	{
		chan_ = channel_create(sizeof(T), 100);
	}

	~channel(void)
	{
		channel_free(chan_);
	}

	channel& operator << (T& t)
	{
		return put(t);
	}

	channel& put(T& t)
	{
		channel_send(chan_, &t);
		return *this;
	}

	void pop(T& t)
	{
		channel_recv(chan_, &t);
	}

private:
	ACL_CHANNEL* chan_;

	channel(const channel&);
	void operator=(const channel&);
};

} // namespace acl
