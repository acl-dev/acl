#pragma once

#ifndef KB
#define KB	(1024)
#endif

#ifndef MB
#define MB	(KB * 1024)
#endif

class CScanDir
{
public:
	CScanDir(const char* path, BOOL nested);
	~CScanDir(void);
protected:
	char* m_pDirPath;
	BOOL  m_nested;
public:
	 int BeginScan(void);
private:
	// 扫描的文件总数
	int m_nFile, m_nDir;
	__int64 m_nSize;
public:
	int FileCount(void);
	int DirCount(void);
	__int64 TotalSize(void);
	int BeginRemove(void);
};
