#pragma once
#include "lib_acl.h"

class CChatDemo
{
public:
	CChatDemo(const char *s);
	~CChatDemo(void);

	ACL_VSTRING *m_buf;
};
