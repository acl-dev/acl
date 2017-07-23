#include "stdafx.h"
#include "pull_mode/message.h"
#include "pull_mode/message_manager.h"

message_manager::message_manager(void)
{
	queue_ = acl_aqueue_new();
}

static void free_message(void* ctx)
{
	message* msg = (message*) ctx;
	delete msg;
}

message_manager::~message_manager(void)
{
	acl_aqueue_free(queue_, free_message);
}

void message_manager::put(message* msg)
{
	if (acl_aqueue_push(queue_, msg) < 0)
		logger_error("push msg failed!");
}

message* message_manager::pop()
{
	message* msg = (message*) acl_aqueue_pop(queue_);
	return msg;
}
