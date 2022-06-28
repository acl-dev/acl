#pragma once

class CIdxTrans
{
public:
	CIdxTrans(void);
	~CIdxTrans(void);
	void Init(HWND hWnd, CString &sPath);
	void Run(void);
	void OnTransing(int nMsg);
	void OnTransEnd(int nMsg);

private:
	HWND m_hWnd;
	CString m_sPath;
	int  m_nMsgTransing;
	int  m_nMsgTransEnd;

	void Trans(void);
	static void *RunThread(void *arg);
};
