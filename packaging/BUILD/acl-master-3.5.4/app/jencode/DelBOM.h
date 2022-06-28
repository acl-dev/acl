#pragma once

class CDelBOM : public acl::thread
{
public:
	CDelBOM(void);
	~CDelBOM(void);

	void Init(HWND hWnd, CString &sPath);
	void OnDeleting(int nMsg);
	void OnDeleted(int nMsg);

protected:
	// @override
	void *run(void);

private:
	HWND m_hWnd;
	CString m_sPath;
	int  m_nMsgDeleting;
	int  m_nMsgDeleted;

	void ScanDel(void);
	bool DeleteBOM(const acl::string& filePath);
};

