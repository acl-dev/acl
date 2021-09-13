
// HttpGetDlg.h: 头文件
//

#pragma once

enum {
	HTTP_DOWNLOAD_FIBER  = 0,
	HTTP_DOWNLOAD_THREAD = 1,
};

// CHttpGetDlg 对话框
class CHttpGetDlg : public CDialogEx
{
// 构造
public:
	CHttpGetDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HTTPGET_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	void InitFiber();

public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedStartGet();
	afx_msg void OnBnClickedReset();
	afx_msg void OnBnClickedRadio();

private:
	CProgressCtrl m_progress;
	CString m_url;
	int m_downType;
	CEdit m_request;
	CEdit m_response;

	long long m_length;
	UINT m_lastPos;

public:
	void SetError(const char* fmt, ...);

	void SetRequestHead(const char* str);
	void SetResponseHead(const char* str);
	void SetBodyTotalLength(long long length);
	void SetBodyLength(long long length);
};
