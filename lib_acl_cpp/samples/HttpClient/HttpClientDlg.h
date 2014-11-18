// HttpClientDlg.h : 头文件
//

#pragma once
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/http/http_service.hpp"

// CHttpClientDlg 对话框
class CHttpClientDlg : public CDialog
{
// 构造
public:
	CHttpClientDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CHttpClientDlg();

// 对话框数据
	enum { IDD = IDD_HTTPCLIENT_DIALOG };

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
	afx_msg void OnBnClickedDownload();
	afx_msg LRESULT OnDownloadOk(WPARAM wParam, LPARAM lParam);

private:
	acl::aio_handle handle_;
	acl::http_service* service_;
};
