#pragma once

struct CHANNEL;
extern "C" {
	extern CHANNEL *channel_create(int elemsize, int bufsize);
	extern void channel_free(CHANNEL *c);
}

namespace acl {

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
	CHANNEL* chan_;
};

} // namespace acl
