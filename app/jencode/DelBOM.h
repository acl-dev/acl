#pragma once

class CDelBOM
{
public:
	CDelBOM(void);
	~CDelBOM(void);

	void Init(HWND hWnd, CString &sPath);
	void Run(void);
	void OnDeleting(int nMsg);
	void OnDeleted(int nMsg);

private:
	HWND m_hWnd;
	CString m_sPath;
	int  m_nMsgDeleting;
	int  m_nMsgDeleted;

	bool DeleteBOM(CString& filePath);
	static void *RunThread(void *arg);
};

