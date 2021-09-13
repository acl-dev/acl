
// HttpGetDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "HttpGet.h"
#include "HttpGetDlg.h"
#include "afxdialogex.h"

#include "HttpClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CHttpGetDlg 对话框

CHttpGetDlg::CHttpGetDlg(CWnd* pParent /*=nullptr*/)
: CDialogEx(IDD_HTTPGET_DIALOG, pParent)
, m_url("http://www.baidu.com/")
, m_length(-1)
, m_lastPos(0)
, m_downType(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CHttpGetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DOWN_PROGRESS, m_progress);
	DDX_Text(pDX, IDC_URL, m_url);
	DDX_Control(pDX, IDC_REQUEST_HEAD, m_request);
	DDX_Control(pDX, IDC_RESPONSE, m_response);
	DDX_Radio(pDX, IDC_RADIO_FIBER, m_downType);
}

BEGIN_MESSAGE_MAP(CHttpGetDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CHttpGetDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CHttpGetDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_START_GET, &CHttpGetDlg::OnBnClickedStartGet)
	ON_BN_CLICKED(IDC_RESET, &CHttpGetDlg::OnBnClickedReset)
	ON_BN_CLICKED(IDC_RADIO_FIBER, &CHttpGetDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_THREAD, &CHttpGetDlg::OnBnClickedRadio)
END_MESSAGE_MAP()


// CHttpGetDlg 消息处理程序

BOOL CHttpGetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr) {
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	InitFiber();

	m_progress.SetRange(0, 100);
	m_progress.SetPos(0);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CHttpGetDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CHttpGetDlg::OnPaint()
{
	if (IsIconic()) {
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CHttpGetDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CHttpGetDlg::InitFiber()
{
	acl::acl_cpp_init();
	// 设置协程调度的事件引擎，同时将协程调度设为自动启动模式，不能在进程初始化时启动
	// 协程调试器，必须在界面消息引擎正常运行后才启动协程调度器！
	acl::fiber::init(acl::FIBER_EVENT_T_WMSG, true);
	acl::fiber::winapi_hook();
}

void CHttpGetDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}

void CHttpGetDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}

void CHttpGetDlg::OnBnClickedStartGet()
{
	// TODO: 在此添加控件通知处理程序代码
	CString* url = new CString;
	GetDlgItem(IDC_URL)->GetWindowText(*url);
	if (url->IsEmpty()) {
		MessageBox("Please input http url first!");
		GetDlgItem(IDC_URL)->SetFocus();
		delete url;
		return;
	}

	m_progress.SetPos(0);
	m_lastPos = 0;

	if (m_downType == HTTP_DOWNLOAD_FIBER) {
		go[=] {
			CHttpClient client(*this, *url);
			delete url;
			client.run();
		};
	} else if (m_downType == HTTP_DOWNLOAD_THREAD) {
		go[=] {
			acl::fiber_tbox<CHttpMsg> box;
			std::thread thread([&] {
				CHttpClient client(box, *url);
				delete url;
				client.run();
			});

			thread.detach();

			while (true) {
				CHttpMsg* msg = box.pop();
				if (msg == NULL) {
					break;
				}
				switch (msg->m_type) {
				case HTTP_MSG_ERR:
					SetError("%s", msg->m_buf.GetString());
					break;
				case HTTP_MSG_REQ:
					SetRequestHead(msg->m_buf.GetString());
					break;
				case HTTP_MSG_RES:
					SetResponseHead(msg->m_buf.GetString());
					break;
				case HTTP_MSG_TOTAL_LENGTH:
					SetBodyTotalLength(msg->m_length);
					break;
				case HTTP_MSG_LENGTH:
					SetBodyLength(msg->m_length);
					break;
				default:
					break;
				}
				delete msg;
			}
		};
	} else {
		delete url;
	}
}

void CHttpGetDlg::OnBnClickedReset()
{
	// TODO: 在此添加控件通知处理程序代码
	m_progress.SetPos(0);
	m_lastPos = 0;
	GetDlgItem(IDC_REQUEST_HEAD)->SetWindowText("");
	GetDlgItem(IDC_RESPONSE)->SetWindowTextA("");
}

void CHttpGetDlg::SetError(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	CString msg;
	msg.FormatV(fmt, ap);
	va_end(ap);

	MessageBox(msg);
}

void CHttpGetDlg::SetRequestHead(const char* str)
{
	m_request.SetWindowText(str);
}

void CHttpGetDlg::SetResponseHead(const char* str)
{
	m_response.SetWindowText(str);
}

void CHttpGetDlg::SetBodyTotalLength(long long length)
{
	m_length = length;
}

void CHttpGetDlg::SetBodyLength(long long length)
{
	if (m_length <= 0) {
		return;
	}

	UINT n = (UINT) (100 * length / m_length);
	if (n >= m_lastPos + 1) {
		m_progress.SetPos(n);
		m_lastPos = n;
	}
}

void CHttpGetDlg::OnBnClickedRadio()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	switch (m_downType) {
	case HTTP_DOWNLOAD_FIBER:
		break;
	case HTTP_DOWNLOAD_THREAD:
		break;
	default:
		break;
	}
}
