#pragma once

class CAclTrans
{
public:
	CAclTrans(void);
	~CAclTrans(void);
	CAclTrans(HWND hWnd, CString &sPath);
	void Init(HWND hWnd, CString &sPath);
	void Run(BOOL bTrans = TRUE);
public:
	void OnTransing(int nMsg);
	void OnTransEnd(int nMsg);
private:
	HWND m_hWnd;
	CString m_sPath;
	int  m_nMsgTransing;
	int  m_nMsgTransEnd;
	BOOL m_bTrans;

	static void *RunThread(void *arg);

	int ScanPath(CString *psPath);
	int TransFile(const char *psPath);
	void Trans(char *psBuf);
	void Restore(char *psBuf);
};
