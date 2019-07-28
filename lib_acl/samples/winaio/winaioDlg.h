// winaioDlg.h : 头文件
//

#pragma once


// CwinaioDlg 对话框
class CwinaioDlg : public CDialog
{
// 构造
public:
	CwinaioDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_WINAIO_DIALOG };

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
public:
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedConnect();
	afx_msg void OnBnClickedListen();
	afx_msg void OnBnClickedSetTimer();
	afx_msg void OnBnClickedDelTimer();
};
