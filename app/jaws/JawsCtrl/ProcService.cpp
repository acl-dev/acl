#include "StdAfx.h"
#include "lib_acl.h"
#include ".\procservice.h"

void CProcService::Init(const char *procname)
{
	m_procName = strdup(procname);
}

CProcService::~CProcService(void)
{
	if (m_procName)
		free(m_procName);
}

void CProcService::DebugArgv(void)
{
	int   i;

	for (i = 0; i < m_argc; i++)
	{
		acl_msg_info("argv[%d]: %s", i, m_argv[i]);
	}
}