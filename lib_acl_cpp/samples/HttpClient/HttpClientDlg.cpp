// HttpClientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "HttpClient.h"
#include "HttpClientDlg.h"
#include "HttpDownload.h"
#include ".\httpclientdlg.h"

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif


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


// CHttpClientDlg 对话框



CHttpClientDlg::CHttpClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHttpClientDlg::IDD, pParent)
	, handle_(acl::ENGINE_WINMSG)
	, service_(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CHttpClientDlg::~CHttpClientDlg()
{
	if (service_)
	{
		delete service_;

		// service_ 会与一个异步IO绑定，当删除后并不会立即关闭
		// 该异步 IO，因为异步引擎采用延迟关闭机制，所以需要调
		// 用 handle_check() 来主动释放延迟释放队列里的异步 IO
		handle_.check();
	}
}

void CHttpClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHttpClientDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_DOWNLOAD, OnBnClickedDownload)
	ON_MESSAGE(WM_USER_DOWNLOAD_OVER, OnDownloadOk)
END_MESSAGE_MAP()


// CHttpClientDlg 消息处理程序

BOOL CHttpClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将\“关于...\”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	FILE *fp;
	AllocConsole();
	fp = freopen("CONOUT$","w+t",stdout);

	service_ = NEW acl::http_service(2, 0, true);
	// 使消息服务器监听 127.0.0.1 的地址
	//if (service_->open(&handle_) == false)
	//{
	//	printf(">>open message service error!\n");
	//	delete service_;
	//	service_ = NULL;
	//}
	
	acl::atomic_long n;
	n++;
	printf("n=%lld\r\n", n.value());
	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void CHttpClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CHttpClientDlg::OnPaint() 
{
	if (IsIconic())
	{
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
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR CHttpClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CHttpClientDlg::OnBnClickedDownload()
{
	// TODO: 在此添加控件通知处理程序代码

	if (service_ == NULL)
	{
		printf("service_ null\r\n");
		return;
	}

	// 创建 HTTP 请求过程
	acl::string domain;
	domain = "www.banmau.com";
	//domain = "192.168.1.229";
	//domain = "www.renwou.net";
	CHttpDownload* req = NEW CHttpDownload(domain.c_str(), 80, &handle_);
	req->SetHWnd(this->GetSafeHwnd());
	//req->set_url("/index.html");
	req->set_url("/download/banma_install.exe");
	req->set_host(domain);
	req->set_keep_alive(false);
	req->set_method(acl::HTTP_METHOD_GET);
	req->add_cookie("x-cookie-name", "cookie-value");
	//req->set_redirect(1); // 设置自动重定向的次数限制

	// 通知异步消息服务器处理该 HTTP 请求过程

	//////////////////////////////////////////////////////////////////////////
	//acl::string buf;
	//req->build_request(buf);
	//printf("-----------------------------------------------\n");
	//printf("%s", buf.c_str());
	//printf("-----------------------------------------------\n");
	//////////////////////////////////////////////////////////////////////////

	GetDlgItem(IDC_DOWNLOAD)->EnableWindow(FALSE);
	service_->do_request(req);
}

LRESULT CHttpClientDlg::OnDownloadOk(WPARAM wParam, LPARAM lParam)
{
	GetDlgItem(IDC_DOWNLOAD)->EnableWindow(TRUE);
	return (0);
}