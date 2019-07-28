#pragma once

struct ACL_AQUEUE;
class message;

class message_manager
{
public:
	message_manager(void);
	~message_manager(void);

	void put(message* msg);
	message* pop();

private:
	ACL_AQUEUE* queue_;
};
