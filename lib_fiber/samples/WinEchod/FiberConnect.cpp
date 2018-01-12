#include "stdafx.h"
#include "FiberConnect.h"

CFiberConnect::CFiberConnect(UINT count)
	: m_count(count)
	, m_sock(INVALID_SOCKET)
{
}

CFiberConnect::~CFiberConnect(void)
{
	if (m_sock != INVALID_SOCKET)
	{
		fiber_close(m_sock);
	}
}

void CFiberConnect::run(void)
{

}