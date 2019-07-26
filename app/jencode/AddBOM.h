#pragma once
class CAddBOM : public acl::thread
{
public:
	CAddBOM(void);
	~CAddBOM(void);

	void Init(HWND hWnd, CString &sPath);
	void OnAdding(int nMsg);
	void OnAdded(int nMsg);

protected:
	// @override
	void *run(void);

private:
	HWND m_hWnd;
	CString m_sPath;
	int  m_nMsgAdding;
	int  m_nMsgAdded;

	void ScanAdd(void);
	bool AddBOM(const acl::string& filePath);
};
