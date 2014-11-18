#pragma once

class CProcService
{
public:
	CProcService(void) : m_procName(NULL) {}
	void Init(const char *procname);
	virtual ~CProcService(void);
	void DebugArgv(void);
public:
	int m_argc;
	char **m_argv;
	char *m_procName;
};
