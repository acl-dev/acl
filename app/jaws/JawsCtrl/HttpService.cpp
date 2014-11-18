#include "StdAfx.h"
#include ".\httpservice.h"

static char *tag_addr = "--addr";
static char *tag_http_vhost_path = "--http_vhost_path";
static char *tag_http_vhost_default = "--http_vhost_default";
static char *tag_http_tmpl_path = "--http_tmpl_path";
static char *tag_http_filter_info = "--http_filter_info";

CHttpService::CHttpService(
	const char * procname,
	const char * addr,
	const char * vhostPath,
	const char * vhostDefault,
	const char * tmplPath,
	const char * filterInfo)
: m_addr(_T(addr))
, m_vhostPath(_T(vhostPath))
, m_vhostDefault(_T(vhostDefault))
, m_tmplPath(_T(tmplPath))
, m_filterInfo(_T(filterInfo))
{
	int  i;

	m_argc = 10;
	m_argv = (char **) calloc(m_argc, sizeof(char*));

	i = 0;

	m_argv[i++] = strdup(tag_addr);
	m_argv[i++] = strdup(addr);

	m_argv[i++] = strdup(tag_http_vhost_path);
	m_argv[i++] = strdup(vhostPath);

	m_argv[i++] = strdup(tag_http_vhost_default);
	m_argv[i++] = strdup(vhostDefault);

	m_argv[i++] = strdup(tag_http_tmpl_path);
	m_argv[i++] = strdup(tmplPath);

	m_argv[i++] = strdup(tag_http_filter_info);
	m_argv[i++] = strdup(filterInfo);

	ASSERT(i == m_argc);
	CProcService::Init(_T(procname));
}

CHttpService::~CHttpService(void)
{
	int i;

	for (i = 0; i < m_argc; i++)
	{
		free(m_argv[i]);
	}
	free(m_argv);
}
