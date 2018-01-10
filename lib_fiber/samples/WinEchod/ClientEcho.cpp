#include "stdafx.h"
#include "ClientEcho.h"


CClientEcho::CClientEcho(acl::socket_stream* conn)
	: m_conn(conn)
{
}

CClientEcho::~CClientEcho(void)
{
}

void CClientEcho::run(void)
{
	acl::string buf;
	while (true)
	{
		if (m_conn->gets(buf) == false)
		{
			printf("gets error\r\n");
			break;
		}
		if (m_conn->write(buf) == -1)
		{
			printf("write error\r\n");
			break;
		}
	}
	delete m_conn;
	delete this;
}