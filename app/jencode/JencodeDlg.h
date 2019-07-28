// JencodeDlg.h : 头文件
//

#pragma once
#include <time.h>

// CJencodeDlg 对话框
class CJencodeDlg : public CDialog
{
// 构造
public:
	CJencodeDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_JENCODE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	CStatusBarCtrl m_wndStatus;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg LRESULT OnTransOver(WPARAM uID, LPARAM lEvent);
	afx_msg LRESULT OnTransOver2(WPARAM uID, LPARAM lEvent);
private:
	time_t  m_nBegin;
	BOOL CheckPath(void);
	void ButtonsEnable(void);
	void ButtonsDisable(void);
	CString m_sPath, m_dPath;
public:
	afx_msg void OnBnClickedAclTrans();
	afx_msg void OnBnClickedAclRestore();
	afx_msg void OnBnClickedIdxSelect();
	afx_msg void OnBnClickedTransIdx();
private:
	CString m_sIdxPath, m_fsPath;
public:
	afx_msg void OnBnClickedDelBom();
	afx_msg void OnBnClickedButtonGb2utf();
	afx_msg void OnBnClickedButtonUtf2gb();
	afx_msg void OnBnClickedButtonGb2uni();
	afx_msg void OnBnClickedAddBom();
};
