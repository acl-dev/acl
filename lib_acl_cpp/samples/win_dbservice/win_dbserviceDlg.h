// win_dbserviceDlg.h : 头文件
//

#pragma once

#include "acl_cpp/db/db_service.hpp"

// Cwin_dbserviceDlg 对话框
class Cwin_dbserviceDlg : public CDialog
{
// 构造
public:
	Cwin_dbserviceDlg(CWnd* pParent = NULL);	// 标准构造函数

	// 析构
	~Cwin_dbserviceDlg();

// 对话框数据
	enum { IDD = IDD_WIN_DBSERVICE_DIALOG };

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
	afx_msg void OnBnClickedAddData();

private:
	acl::db_service* server_;
	acl::aio_handle* handle_;
public:
	afx_msg void OnBnClickedGetData();
	afx_msg void OnBnClickedDeleteData();
};
