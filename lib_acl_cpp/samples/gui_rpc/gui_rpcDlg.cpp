// gui_rpcDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "gui_rpc.h"
#include "rpc/rpc_manager.h"
#include "rpc/http_download.h"
#include "gui_rpcDlg.h"

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


// Cgui_rpcDlg 对话框



Cgui_rpcDlg::Cgui_rpcDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cgui_rpcDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

Cgui_rpcDlg::~Cgui_rpcDlg()
{
}

void Cgui_rpcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REQ, m_reqCtlEdit);
	DDX_Control(pDX, IDC_RES, m_resCtlEdit);
	DDX_Text(pDX, IDC_IP, m_serverIp);
	DDX_Text(pDX, IDC_PORT, m_serverPort);
}

BEGIN_MESSAGE_MAP(Cgui_rpcDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_RUN, OnBnClickedButtonRun)
	ON_BN_CLICKED(IDC_CLEAR, OnBnClickedClear)
	ON_WM_SIZE()
	ON_EN_CHANGE(IDC_URL, OnEnChangeUrl)
END_MESSAGE_MAP()


// Cgui_rpcDlg 消息处理程序

BOOL Cgui_rpcDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	BOOL bOk = m_resizer.Hook(this);
	ASSERT(bOk == TRUE);

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

	bOk = m_resizer.SetAnchor(IDC_URL, ANCHOR_HORIZONTALLY);
	ASSERT( bOk);

	bOk = m_resizer.CreateSplitContainer(_T("MySplitter"), IDC_STATIC_REQ, IDC_STATIC_RES);
	ASSERT( bOk );

	//bOk = m_resizer.SetAnchor(IDC_STATIC_REQ, ANCHOR_ALL | ANCHOR_PRIORITY_RIGHT);
	//ASSERT( bOk );
	//bOk = m_resizer.SetAnchor(IDC_STATIC_RES, ANCHOR_ALL | ANCHOR_PRIORITY_BOTTOM);
	//ASSERT( bOk );

	//bOk = m_resizer.SetMinimumSize(IDC_STATIC_REQ, CSize(150, 100));
	//ASSERT( bOk );
	//bOk = m_resizer.SetMinimumSize(IDC_STATIC_RES, CSize(150, 100));
	//ASSERT( bOk );

	bOk = m_resizer.SetAnchor(_T("MySplitter"), ANCHOR_ALL);
	ASSERT( bOk );

	bOk = m_resizer.SetShowSplitterGrip(_T("MySplitter"), TRUE);
	ASSERT( bOk);

	bOk = m_resizer.SetParent(IDC_REQ, IDC_STATIC_REQ);
	ASSERT( bOk);
	bOk = m_resizer.SetParent(IDC_RES, IDC_STATIC_RES);
	ASSERT( bOk);

	bOk = m_resizer.SetAnchor(IDC_REQ, ANCHOR_LEFT | ANCHOR_ALL );
	ASSERT( bOk);
	bOk = m_resizer.SetAnchor(IDC_RES, ANCHOR_RIGHT | ANCHOR_ALL);
	ASSERT( bOk);

	bOk = m_resizer.SetAnchor(IDOK, ANCHOR_RIGHT | ANCHOR_BOTTOM);
	ASSERT(bOk == TRUE);
	bOk = m_resizer.SetAnchor(IDC_BUTTON_RUN, ANCHOR_RIGHT | ANCHOR_BOTTOM);
	ASSERT(bOk == TRUE);
	bOk = m_resizer.SetAnchor(IDC_CLEAR, ANCHOR_RIGHT | ANCHOR_BOTTOM);
	ASSERT(bOk == TRUE);

	//GetClientRect(&m_rect);

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	GetDlgItem(IDC_PORT)->SetWindowText("80");

	// 添加状态栏
	int aWidths[3] = {50, 300, -1};
	m_wndMeterBar.SetParts(3, aWidths);

	m_wndMeterBar.Create(WS_CHILD | WS_VISIBLE | WS_BORDER
		| CCS_BOTTOM | SBARS_SIZEGRIP,
		CRect(0,0,0,0), this, 0); 
	m_wndMeterBar.SetText("就绪", 0, 0);
	m_wndMeterBar.SetText("", 1, 0);
	m_wndMeterBar.SetText("", 2, 0);

	//ShowWindow(SW_MINIMIZE);

	// TODO: 在此添加额外的初始化代码
	
	m_resizer.SetShowResizeGrip(TRUE);

	bOk = m_resizer.InvokeOnResized();
	ASSERT( bOk);

	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void Cgui_rpcDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void Cgui_rpcDlg::OnPaint() 
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

	// 设置 URL 栏焦点
	GetDlgItem(IDC_URL)->SetFocus();
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR Cgui_rpcDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Cgui_rpcDlg::OnBnClickedButtonRun()
{
	// TODO: 在此添加控件通知处理程序代码
	CString url;
	GetDlgItem(IDC_URL)->GetWindowText(url);
	if (url.GetLength() == 0)
	{
		MessageBox("请输入正确的 URL!");
		return;
	}
	const char* ptr = url.GetString();
	if (strncasecmp(ptr, "http://", sizeof("http://") - 1) != 0)
		url = "http://" + url;

	CString addr;
	GetDlgItem(IDC_IP)->GetWindowText(addr);
	if (addr.GetLength() == 0)
	{
		MessageBox("请输入正确的服务器地址!");
		return;
	}

	CString port;
	GetDlgItem(IDC_PORT)->GetWindowText(port);

	addr.AppendFormat(":%s", port.GetString());

	// 在主线程中创建一个 HTTP 下载过程
	http_download* down = new http_download(addr.GetString(),
		url.GetString(), this);

	// 通过子线程开始一个 HTTP 下载过程
	rpc_manager::get_instance().fork(down);

	// 虽然可以同时发起多个 HTTP 下载过程，但此处为了简单，先禁止下一个
	// HTTP 下载任务，直至上一个任务完毕
	GetDlgItem(IDC_BUTTON_RUN)->EnableWindow(false);
}

void Cgui_rpcDlg::OnBnClickedClear()
{
	// TODO: 在此添加控件通知处理程序代码
	m_reqCtlEdit.SetSel(0, m_reqCtlEdit.GetWindowTextLength());
	m_reqCtlEdit.ReplaceSel("");
	m_resCtlEdit.SetSel(0, m_resCtlEdit.GetWindowTextLength());
	m_resCtlEdit.ReplaceSel("");
	m_wndMeterBar.GetProgressCtrl().SetPos(0);
}

void Cgui_rpcDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	//创建对话框时，控件还没有创建，因此不能改变其大小(必须加上这两行代码)
	if(!IsWindowVisible())
		return;

	// TODO: 在此处添加消息处理程序代码
	if (m_wndMeterBar.GetSafeHwnd())
	{
		int aWidths[3] = {cx / 3, cx / 3, -1};
		m_wndMeterBar.SetParts(3, aWidths);
		m_wndMeterBar.SetText("就绪", 0, 0);
		m_wndMeterBar.SetText("就绪2", 1, 0); //SBT_NOBORDERS);
		m_wndMeterBar.SetText("", 2, SBT_NOBORDERS);

		m_wndMeterBar.SendMessage(WM_SIZE, nType, MAKELONG(cy, cx));
	}
}

void Cgui_rpcDlg::OnEnChangeUrl()
{
	// TODO:  如果该控件是 RICHEDIT 控件，则它将不会
	// 发送该通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString url;
	GetDlgItem(IDC_URL)->GetWindowText(url);
	if (url.GetLength() == 0)
		return;
	url.MakeLower();
	char* ptr = url.GetBuffer();
#define SKIP_SP(x) do {	if (*(x) == ' ' || *(x) == '\t') (x)++; } while(0)
	SKIP_SP(ptr);
	if (*ptr == 0 || strlen(ptr) <= sizeof("http://") - 1)
		return;
	if (strncmp(ptr, "http://", sizeof("http://") - 1) == 0)
		ptr += sizeof("http://") - 1;
	if (*ptr == 0)
	{
		GetDlgItem(IDC_IP)->SetWindowText("");
		return;
	}
	CString addr = ptr;
	int pos = addr.Find('/');
	if (pos >= 0)
		addr.Truncate(pos);
	CString port = "80";
	pos = addr.Find(':');
	if (pos >= 0)
	{
		port = addr.Mid(pos + 1);
		addr.Truncate(pos);
	}
	GetDlgItem(IDC_IP)->SetWindowText(addr.GetString());
	if (port.GetLength() > 0)
		GetDlgItem(IDC_PORT)->SetWindowText(port.GetString());
}

//////////////////////////////////////////////////////////////////////////

void Cgui_rpcDlg::OnDownloading(long long int content_length,
	long long int total_read)
{
	if (content_length > 0)
	{
		int  nStept;

		nStept = (int) ((total_read * 100) / (content_length));
		m_wndMeterBar.GetProgressCtrl().SetPos(nStept);
	}
 
	CString msg;
	msg.Format("共 %I64d 字节", total_read);
	m_wndMeterBar.SetText(msg, 1, 0);
}

void Cgui_rpcDlg::OnDownloadOver(long long int total_read, double spent)
{
	CString msg;
	msg.Format("共 %I64d 字节，耗时 %.3f 毫秒", total_read, spent);
	m_wndMeterBar.SetText(msg, 1, 0);
	GetDlgItem(IDC_BUTTON_RUN)->EnableWindow(true);
}

void Cgui_rpcDlg::SetRequestHdr(const char* hdr)
{
	if (hdr == NULL || *hdr == 0)
		return;
	m_reqCtlEdit.SetSel(0, m_reqCtlEdit.GetWindowTextLength());
	m_reqCtlEdit.ReplaceSel(hdr);
}

void Cgui_rpcDlg::SetResponseHdr(const char* hdr)
{
	if (hdr == NULL || *hdr == 0)
		return;
	m_resCtlEdit.SetSel(0, m_resCtlEdit.GetWindowTextLength());
	m_resCtlEdit.ReplaceSel(hdr);
}
