#pragma once

struct ACL_CHANNEL;
extern "C" {
	extern ACL_CHANNEL *acl_channel_create(int elemsize, int bufsize);
	extern void acl_channel_free(ACL_CHANNEL *c);
	extern int acl_channel_send(ACL_CHANNEL *c, void *v);
	extern int acl_channel_recv(ACL_CHANNEL *c, void *v);
}

namespace acl {

template <typename T>
class channel
{
public:
	channel(void)
	{
		chan_ = acl_channel_create(sizeof(T), 100);
	}

	~channel(void)
	{
		acl_channel_free(chan_);
	}

	channel& operator << (T& t)
	{
		return put(t);
	}

	channel& put(T& t)
	{
		acl_channel_send(chan_, &t);
		return *this;
	}

	void pop(T& t)
	{
		acl_channel_recv(chan_, &t);
	}

private:
	ACL_CHANNEL* chan_;
};

} // namespace acl
