#include "stdafx.hpp"
#include "fiber/channel.hpp"

namespace acl
{

ACL_CHANNEL *channel_create(int elemsize, int bufsize)
{
	return acl_channel_create(elemsize, bufsize);
}

void channel_free(ACL_CHANNEL *c)
{
	return acl_channel_free(c);
}

int channel_send(ACL_CHANNEL *c, void *v)
{
	return acl_channel_send(c, v);
}

int channel_recv(ACL_CHANNEL *c, void *v)
{
	return acl_channel_recv(c, v);
}

}
