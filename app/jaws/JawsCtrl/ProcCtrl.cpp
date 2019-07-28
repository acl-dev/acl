#include "StdAfx.h"
#include ".\procctrl.h"

CProcCtrl::CProcCtrl(void)
{
	char  logfile[MAX_PATH], *ptr;

	acl_proctl_daemon_path(m_exePath, sizeof(m_exePath));
	ptr = strrchr(m_exePath, '\\');
	if (ptr == NULL)
		ptr = strrchr(m_exePath, '/');

	if (ptr == NULL)
		ptr = m_exePath;
	else
		ptr++;

	snprintf(logfile, sizeof(logfile), "%s/JawsCtrl.log", m_exePath);
	acl_msg_open(logfile, "daemon");
	acl_debug_init("all:2");
}

CProcCtrl::~CProcCtrl(void)
{
}

static void *daemon_loop(void *arg)
{
	acl_proctl_daemon_loop();
	/* not reached */
	return (NULL);
}

void CProcCtrl::RunThread()
{
	acl_pthread_t tid;
	acl_pthread_attr_t attr;

	acl_proctl_deamon_init(m_exePath);
	acl_pthread_attr_init(&attr);
	acl_pthread_create(&tid, &attr, daemon_loop, NULL);
}

void *CProcCtrl::StartThread(void *arg)
{
	CProcCtrl *ctrl = (CProcCtrl*) arg;

	acl_proctl_start_one(ctrl->m_exePath, ctrl->m_procName,
		ctrl->m_service.m_argc, ctrl->m_service.m_argv);
	return (NULL);
}

void CProcCtrl::StartOne(CProcService& service)
{
#undef	USE_THREAD_CTRL

#ifdef	USE_THREAD_CTRL
	m_service = service;
	ACL_SAFE_STRNCPY(m_procName, service.m_procName, sizeof(m_procName));
	acl_pthread_t tid;
	acl_pthread_create(&tid, NULL, StartThread, this);
#else
	acl_proctl_deamon_start_one(service.m_procName,
		service.m_argc, service.m_argv);
#endif
}

void CProcCtrl::StopOne(CProcService& service)
{
	acl_proctl_stop_one(m_exePath, service.m_procName, 0, NULL);
}