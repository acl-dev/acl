// FileDirDlg.h : 头文件
//

#pragma once

#include "lib_acl.h"
#include "afxcmn.h"
#include "afxwin.h"

// CFileDirDlg 对话框
class CFileDirDlg : public CDialog
{
// 构造
public:
	CFileDirDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_FILEDIR_DIALOG };

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
	CString m_dirPath;
	BOOL m_nested;
private:
	int GetDirPath(void);
public:
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnEnChangeEditPath();
private:
	ACL_SCAN_DIR *m_pScan;
	static int ScanCallback(ACL_SCAN_DIR * scan, void * ctx);
	static int ScanSvnCallback(ACL_SCAN_DIR * scan, void * ctx);
	void ScanDir(void);
	void ScanSvnDir(void);
	// 更新显示信息
	void UpdateInfo(void);
	BOOL UpdateSvn(const char* filepath);
	void ScanClose(void);
public:
	afx_msg void OnBnClickedCheckNetsted();
	afx_msg void OnBnClickedButtonScan();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedButtonAvlAdd();
	afx_msg void OnBnClickedButtonAvlFind();
private:
	avl_tree_t m_avlTree;
public:
	static int compare_fn(const void* v1, const void* v2);
	afx_msg void OnBnClickedButtonWalk();
public:
	afx_msg void OnEnChangeEditAvlName();
public:
	afx_msg void OnEnChangeRichedit2AvlResult();
	afx_msg void OnBnClickedButtonTest();
private:
	CEdit m_debugWin;
public:
	afx_msg void OnEnChangeEditAvlValue();
private:
	CString m_avlValue;
	CString m_avlName;
public:
	afx_msg void OnBnClickedButtonAvlAddBat();
	afx_msg void OnBnClickedButtonAvlDel();
	afx_msg void OnBnClickedButtonClear();
private:
	void DebugWinAppend(CString& msg);
public:
	// 是否将扫描目录的文件名输出
	BOOL m_outName;
	afx_msg void OnBnClickedCheckOutName();
	afx_msg void OnBnClickedSvnUpdate();
};
