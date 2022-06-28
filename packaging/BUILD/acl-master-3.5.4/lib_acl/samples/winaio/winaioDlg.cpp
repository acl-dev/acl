// winaioDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "winaio.h"
#include "lib_acl.h"
#include "aio_client.h"
#include "aio_server.h"
#include "winaioDlg.h"
#include ".\winaiodlg.h"
//#include "vld.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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


// CwinaioDlg 对话框



CwinaioDlg::CwinaioDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CwinaioDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CwinaioDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CwinaioDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_START, OnBnClickedStart)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_CONNECT, OnBnClickedConnect)
	ON_BN_CLICKED(IDC_LISTEN, OnBnClickedListen)
	ON_BN_CLICKED(IDC_SET_TIMER, OnBnClickedSetTimer)
	ON_BN_CLICKED(IDC_DEL_TIMER, OnBnClickedDelTimer)
END_MESSAGE_MAP()

static ACL_AIO *__aio = NULL;

// CwinaioDlg 消息处理程序

BOOL CwinaioDlg::OnInitDialog()
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
	//acl_init();
	//VLDEnable();
	aio_client_init();
	__aio = acl_aio_create(ACL_EVENT_WMSG);
	ASSERT(__aio);
	
	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void CwinaioDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CwinaioDlg::OnPaint() 
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
HCURSOR CwinaioDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CwinaioDlg::OnBnClickedStart()
{
	// TODO: 在此添加控件通知处理程序代码

	const char *addr = "127.0.0.1:30082";
	aio_client_start(__aio, addr, 100);
}

void CwinaioDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	if (__aio)
	{
		aio_server_end();
		acl_aio_free(__aio);
		acl_pthread_end();
		__aio = NULL;
	}
	OnOK();
}

void CwinaioDlg::OnBnClickedConnect()
{
	// TODO: 在此添加控件通知处理程序代码

	const char *addr = "127.0.0.1:30082";
	aio_client_start(__aio, addr, 10);
}

void CwinaioDlg::OnBnClickedListen()
{
	// TODO: 在此添加控件通知处理程序代码
	const char *addr = "127.0.0.1:30082";
	aiho_server_start(__aio, addr,  1, 0);
}

typedef struct
{
	int  i;
	ACL_AIO *aio;
} CTX;

static void OnTimerCallback(int event_type, ACL_EVENT *event, void *context)
{
	CTX *ctx = (CTX*) context;

	ASSERT(ctx->aio == __aio);
	printf(">>>timer %d get now\r\n", ctx->i);
	free(ctx);
}

static CTX *__last_ctx;

void CwinaioDlg::OnBnClickedSetTimer()
{
	// TODO: 在此添加控件通知处理程序代码
	CTX *ctx = NULL;

	for (int i = 0; i < 10; i++)
	{
		ctx = (CTX*) malloc(sizeof(CTX));
		ctx->aio = __aio;
		ctx->i = i;
		printf("set timer %d\n", i);
		acl_aio_request_timer(__aio, OnTimerCallback, ctx, i + 1, 1);
	}
	__last_ctx = ctx;
}

void CwinaioDlg::OnBnClickedDelTimer()
{
	// TODO: 在此添加控件通知处理程序代码
	ASSERT(__last_ctx);
	printf("cancel timer %d\n", __last_ctx->i);
	acl_aio_cancel_timer(__aio, OnTimerCallback, __last_ctx);
	free(__last_ctx);
}
