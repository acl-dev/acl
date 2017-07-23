#include "stdafx.h"
#include "pull_mode/message.h"

message::message(const acl::string& server)
: server_(server)
{
}

void message::add(const char* data, size_t len)
{
	buf_.append(data, len);
}
