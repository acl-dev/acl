// JencodeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Jencode.h"
#include "Gb2Utf8.h"
#include "AclTrans.h"
#include "IdxTrans.h"
#include "JencodeDlg.h"
#include "DelBOM.h"
#include "AddBOM.h"
#include "JencodeDlg.h"
#include ".\jencodedlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_USER_TRANS_OVER	WM_USER + 100
#define WM_USER_TRANS_OVER2	WM_USER + 101

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CJencodeDlg 对话框



CJencodeDlg::CJencodeDlg(CWnd* pParent /*=NULL*/)
: CDialog(CJencodeDlg::IDD, pParent)
, m_sIdxPath(_T(""))
, m_fsPath(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CJencodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CJencodeDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_GB2UTF, OnBnClickedButtonGb2utf)
	ON_BN_CLICKED(IDC_BUTTON_UTF2GB, OnBnClickedButtonUtf2gb)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
	ON_MESSAGE(WM_USER_TRANS_OVER, OnTransOver)
	ON_MESSAGE(WM_USER_TRANS_OVER2, OnTransOver2)
	ON_BN_CLICKED(IDC_ACL_TRANS, OnBnClickedAclTrans)
	ON_BN_CLICKED(IDC_ACL_RESTORE, OnBnClickedAclRestore)
	ON_BN_CLICKED(IDC_IDX_SELECT, OnBnClickedIdxSelect)
	ON_BN_CLICKED(IDC_TRANS_IDX, OnBnClickedTransIdx)
	ON_BN_CLICKED(IDC_DEL_BOM, OnBnClickedDelBom)
	ON_BN_CLICKED(IDC_BUTTON_GB2UNI, &CJencodeDlg::OnBnClickedButtonGb2uni)
	ON_BN_CLICKED(IDC_ADD_BOM, &CJencodeDlg::OnBnClickedAddBom)
END_MESSAGE_MAP()


// CJencodeDlg 消息处理程序

BOOL CJencodeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将\“关于...\”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL) {
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	acl::log::open("jencode.log", "jencode");
	logger("started!");

	//freopen("CONOUT$","w+t",stdout);
	// 添加状态栏
	int aWidths[2] = {50, -1};
	m_wndStatus.Create(WS_CHILD | WS_VISIBLE | WS_BORDER
		| CCS_BOTTOM | SBARS_SIZEGRIP,
		CRect(0,0,0,0), this, 0);
	m_wndStatus.SetParts(2, aWidths);
	m_wndStatus.SetText("就绪", 0, 0);
	m_wndStatus.SetText("", 1, 0);
	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void CJencodeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CJencodeDlg::OnPaint() 
{
	if (IsIconic()) {
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR CJencodeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CJencodeDlg::ButtonsEnable(void)
{
	GetDlgItem(IDC_BUTTON_GB2UTF)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_UTF2GB)->EnableWindow(TRUE);
	GetDlgItem(IDC_ACL_TRANS)->EnableWindow(TRUE);
	GetDlgItem(IDC_ACL_RESTORE)->EnableWindow(TRUE);
	GetDlgItem(IDC_DEL_BOM)->EnableWindow(TRUE);
	GetDlgItem(IDC_ADD_BOM)->EnableWindow(TRUE);
}

BOOL CJencodeDlg::CheckPath(void)
{
	UpdateData(TRUE);
	GetDlgItem(IDC_EDIT_SPATH)->GetWindowText(m_sPath);
	//MessageBox(m_sPath);
	if (m_sPath.GetLength() == 0) {
		MessageBox("请选择源目录...");
		return FALSE;
	}

	m_dPath = m_sPath;

	//GetDlgItem(IDC_EDIT_DPATH)->GetWindowText(m_dPath);
	//if (m_dPath.GetLength() == 0)
	//{
	//	MessageBox("请选择目的目录...");
	//	return FALSE;
	//}

	return TRUE;
}

void CJencodeDlg::ButtonsDisable(void)
{
	GetDlgItem(IDC_BUTTON_GB2UTF)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_UTF2GB)->EnableWindow(FALSE);
	GetDlgItem(IDC_ACL_TRANS)->EnableWindow(FALSE);
	GetDlgItem(IDC_ACL_RESTORE)->EnableWindow(FALSE);
	GetDlgItem(IDC_DEL_BOM)->EnableWindow(FALSE);
	GetDlgItem(IDC_ADD_BOM)->EnableWindow(FALSE);
}

void CJencodeDlg::OnBnClickedButtonGb2utf()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!CheckPath()) {
		return;
	}

	static CGb2Utf8 gb2Utf8("gbk", "UTF-8");

	gb2Utf8.Init(this->GetSafeHwnd(), m_sPath, m_dPath);
	gb2Utf8.OnTransEnd(WM_USER_TRANS_OVER);
	gb2Utf8.start();
	m_wndStatus.SetText("运行", 0, 0);
	m_nBegin = time(NULL);
	ButtonsDisable();
}

void CJencodeDlg::OnBnClickedButtonUtf2gb()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!CheckPath()) {
		MessageBox(m_sPath);
		return;
	}

	static CGb2Utf8 utf2gb("utf-8", "gbk");

	utf2gb.Init(this->GetSafeHwnd(), m_sPath, m_dPath);
	utf2gb.OnTransEnd(WM_USER_TRANS_OVER);
	utf2gb.start();
	m_wndStatus.SetText("运行", 0, 0);
	m_nBegin = time(NULL);
	ButtonsDisable();
}

void CJencodeDlg::OnBnClickedButtonGb2uni()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!CheckPath()) {
		MessageBox(m_sPath);
		return;
	}

	return;

	static CGb2Utf8 gb2uni("gbk", "UCS2LE");

	gb2uni.Init(this->GetSafeHwnd(), m_sPath, m_dPath);
	gb2uni.OnTransEnd(WM_USER_TRANS_OVER);
	gb2uni.start();
	m_wndStatus.SetText("运行", 0, 0);
	m_nBegin = time(NULL);
	ButtonsDisable();
}

void CJencodeDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString sPath;
	BROWSEINFO   bi;
	char name[MAX_PATH];
	ZeroMemory(&bi, sizeof(BROWSEINFO));

	bi.hwndOwner = GetSafeHwnd();
	bi.pszDisplayName = name;
	bi.lpszTitle = "Select folder";
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	LPITEMIDLIST idl = SHBrowseForFolder(&bi);
	if(idl == NULL)
		return;
	SHGetPathFromIDList(idl, sPath.GetBuffer(MAX_PATH));  

	sPath.ReleaseBuffer();  

	if (sPath.Right(1) != "\\") {  
		sPath += "\\";
	}

	GetDlgItem(IDC_EDIT_SPATH)->SetWindowText(sPath);
//	CFileDialog file(TRUE,"文件","result.txt",OFN_HIDEREADONLY,"FILE(*.*)|*.*||",NULL);
//	if(file.DoModal()==IDOK)
//	{
//		CString pathname;
//
//		pathname=file.GetPathName();
//		GetDlgItem(IDC_EDIT_SPATH)->SetWindowText(pathname);
//	}
}

void CJencodeDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	CString sPath;
	BROWSEINFO   bi;
	char name[MAX_PATH];
	ZeroMemory(&bi, sizeof(BROWSEINFO));

	bi.hwndOwner = GetSafeHwnd();
	bi.pszDisplayName = name;
	bi.lpszTitle = "Select folder";
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	LPITEMIDLIST idl = SHBrowseForFolder(&bi);
	if(idl == NULL)
		return;
	SHGetPathFromIDList(idl, sPath.GetBuffer(MAX_PATH));  

	sPath.ReleaseBuffer();  

	if (sPath.Right(1) != "\\") {  
		sPath += "\\";
	}

	GetDlgItem(IDC_EDIT_DPATH)->SetWindowText(sPath);
}

afx_msg LRESULT CJencodeDlg::OnTransOver(WPARAM uID, LPARAM lEvent)
{
	CString msg;

	ButtonsEnable();
	msg.Format("耗时：%d 秒", time(NULL) - m_nBegin);
	m_wndStatus.SetText("完成", 0, 0);
	m_wndStatus.SetText(msg, 1, 0);
	return 0;
}

afx_msg LRESULT CJencodeDlg::OnTransOver2(WPARAM uID, LPARAM lEvent)
{
	CString msg;

	GetDlgItem(IDC_TRANS_IDX)->EnableWindow(TRUE);
	msg.Format("耗时：%d 秒", time(NULL) - m_nBegin);
	m_wndStatus.SetText("完成!", 0, 0);
	m_wndStatus.SetText(msg, 1, 0);
	return 0;
}

void CJencodeDlg::OnBnClickedAclTrans()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!CheckPath()) {
		return;
	}

	static CAclTrans aclTrans;

	aclTrans.Init(this->GetSafeHwnd(), m_sPath);
	aclTrans.OnTransEnd(WM_USER_TRANS_OVER);
	aclTrans.Run();
	m_wndStatus.SetText("运行", 0, 0);
	m_nBegin = time(NULL);
	ButtonsDisable();
}

void CJencodeDlg::OnBnClickedAclRestore()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!CheckPath()) {
		return;
	}

	static CAclTrans aclTrans;

	aclTrans.Init(this->GetSafeHwnd(), m_sPath);
	aclTrans.OnTransEnd(WM_USER_TRANS_OVER);
	aclTrans.Run(FALSE);
	m_wndStatus.SetText("运行", 0, 0);
	m_nBegin = time(NULL);
	ButtonsDisable();
}

void CJencodeDlg::OnBnClickedDelBom()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!CheckPath()) {
		return;
	}

	static CDelBOM delBom;
	delBom.Init(this->GetSafeHwnd(), m_sPath);
	delBom.OnDeleted(WM_USER_TRANS_OVER);
	delBom.start();

	m_wndStatus.SetText("运行", 0, 0);
	m_nBegin = time(NULL);
	ButtonsDisable();
}

void CJencodeDlg::OnBnClickedAddBom()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!CheckPath()) {
		return;
	}

	static CAddBOM addBom;
	addBom.Init(this->GetSafeHwnd(), m_sPath);
	addBom.OnAdded(WM_USER_TRANS_OVER);
	addBom.start();

	m_wndStatus.SetText("运行", 0, 0);
	m_nBegin = time(NULL);
	ButtonsDisable();
}

void CJencodeDlg::OnBnClickedIdxSelect()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog file(TRUE,"文件","search.idx",OFN_HIDEREADONLY,"FILE(*.*)|*.*||",NULL);
	if(file.DoModal()==IDOK) {
		CString pathname;

		pathname=file.GetPathName();
		GetDlgItem(IDC_IDX_PATH)->SetWindowText(pathname);
	}
}

void CJencodeDlg::OnBnClickedTransIdx()
{
	// TODO: 在此添加控件通知处理程序代码
	static CIdxTrans idxTrans;

	UpdateData(TRUE);
	GetDlgItem(IDC_IDX_PATH)->GetWindowText(m_fsPath);
	if (m_fsPath.GetLength() == 0) {
		MessageBox("请选择索引文件...");
		return;
	}

	GetDlgItem(IDC_TRANS_IDX)->EnableWindow(FALSE);
	idxTrans.Init(this->GetSafeHwnd(), m_fsPath);
	idxTrans.OnTransEnd(WM_USER_TRANS_OVER2);
	idxTrans.Run();
	m_wndStatus.SetText("运行", 0, 0);
	m_nBegin = time(NULL);
}
