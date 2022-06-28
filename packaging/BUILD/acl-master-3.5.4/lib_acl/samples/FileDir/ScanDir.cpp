//#include "StdAfx.h"

#include "lib_acl.h"
#include ".\scandir.h"

CScanDir::CScanDir(const char* path, BOOL nested)
: m_pDirPath(NULL)
, m_nested(nested)
, m_nDir(0)
, m_nFile(0)
, m_nSize(0)
{
	m_pDirPath = (char*) acl_mystrdup(path);
}

CScanDir::~CScanDir(void)
{
	acl_myfree(m_pDirPath);
}

int CScanDir::BeginScan(void)
{
	m_nSize = acl_scan_dir_size(m_pDirPath, m_nested, &m_nFile, &m_nDir);
	if (m_nSize < 0)
		return (-1);
	return (0);
}

__int64 CScanDir::TotalSize(void)
{
	return (m_nSize);
}

int CScanDir::FileCount(void)
{
	return m_nFile;
}

int CScanDir::DirCount(void)
{
	return m_nDir;
}

int CScanDir::BeginRemove(void)
{
	ACL_SCAN_DIR *scan;

	scan = acl_scan_dir_open(m_pDirPath, m_nested);
	if (scan == NULL)
		return (-1);

	acl_scan_dir_rm2(scan);

	m_nSize = acl_scan_dir_nsize(scan);
	m_nDir = acl_scan_dir_ndirs(scan);
	m_nFile = acl_scan_dir_nfiles(scan);

	acl_scan_dir_close(scan);
	return 0;
}
