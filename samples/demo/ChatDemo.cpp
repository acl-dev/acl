#include "stdafx.h"
#include ".\chatdemo.h"

CChatDemo::CChatDemo(const char *s)
{
	m_buf = acl_vstring_alloc(256);
	acl_vstring_strcpy(m_buf, s);
}

CChatDemo::~CChatDemo(void)
{
	acl_vstring_free(m_buf);
}
