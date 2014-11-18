// win_dbserviceDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "acl_cpp/db/db_service_sqlite.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_sqlite.hpp"
#include "win_dbservice.h"
#include "win_dbserviceDlg.h"
#include ".\win_dbservicedlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////


class myquery : public acl::db_query
{
public:
	myquery(int id) : id_(id)
	{

	}

	~myquery()
	{

	}

	// 基类虚接口：当 SQL 语句出错时的回调函数
	virtual void on_error(acl::db_status status)
	{
		(void) status;
		printf(">>on error, id: %d\r\n", id_);
	}

	// 基类虚接口：当 SQL 语句成功时的回调函数
	virtual void on_ok(const acl::db_rows* rows, int affected)
	{
		if (rows)
			printf(">>on ok, id: %d, rows->legnth: %u, group_name: %s\r\n",
				id_, rows->length(), (*(*rows)[0])["group_name"]);
		else
			printf(">>on ok, id: %d, affected: %d\r\n",
				id_, affected);
	}

	// 基类虚接口：当该类实例对象被释放时的回调函数
	virtual void destroy()
	{
		printf(">> myquery destroy now\r\n");
		delete this;
	}
protected:
private:
	int   id_;
};

static acl::string __dbfile("测试.db");

const char* CREATE_TBL =
"create table group_tbl\r\n"
"(\r\n"
"group_name varchar(128) not null,\r\n"
"uvip_tbl varchar(32) not null default 'uvip_tbl',\r\n"
"access_tbl varchar(32) not null default 'access_tbl',\r\n"
"access_week_tbl varchar(32) not null default 'access_week_tbl',\r\n"
"access_month_tbl varchar(32) not null default 'access_month_tbl',\r\n"
"update_date date not null default '1970-1-1',\r\n"
"disable integer not null default 0,\r\n"
"add_by_hand integer not null default 0,\r\n"
"class_level integer not null default 0,\r\n"
"primary key(group_name, class_level)\r\n"
")";

static bool tbl_create(acl::db_handle& db)
{
	if (db.tbl_exists("group_tbl"))
		return (true);
	if (db.sql_update(CREATE_TBL) == false)
	{
		printf("sql error\r\n");
		return (false);
	}
	else
	{
		printf("create table ok\r\n");
		return (true);
	}
}

static bool create_db(void)
{
	acl::db_sqlite db(__dbfile);

	if (db.open() == false)
	{
		printf("open dbfile: %s error\r\n", __dbfile.c_str());
		return (false);
	}
	db.show_conf();
	return (tbl_create(db));
}

//////////////////////////////////////////////////////////////////////////

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


// Cwin_dbserviceDlg 对话框



Cwin_dbserviceDlg::Cwin_dbserviceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cwin_dbserviceDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	server_ = NULL;
	handle_ = new acl::aio_handle(acl::ENGINE_WINMSG);
}

Cwin_dbserviceDlg::~Cwin_dbserviceDlg()
{
	if (server_)
		delete server_;
	delete handle_;
}

void Cwin_dbserviceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cwin_dbserviceDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_ADD_DATA, OnBnClickedAddData)
	ON_BN_CLICKED(IDC_GET_DATA, OnBnClickedGetData)
	ON_BN_CLICKED(IDC_DELETE_DATA, OnBnClickedDeleteData)
END_MESSAGE_MAP()


// Cwin_dbserviceDlg 消息处理程序

BOOL Cwin_dbserviceDlg::OnInitDialog()
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

	ShowWindow(SW_MINIMIZE);

	// TODO: 在此添加额外的初始化代码
	
	// 打开 DOS 窗口
	AllocConsole();
	FILE* fp = freopen("CONOUT$","w+t",stdout);
	// 打开库的 DOS 窗口
	acl::open_dos();

	logger_open("dbservice.log", "dbservice", "all:1");
	if (create_db() == false)
		printf(">>create table error\r\n");

	// 采用基于 WIN32 消息模式的IPC方式
	server_ = new acl::db_service_sqlite("DB_TEST", __dbfile, 2, 2, true);
	if (server_->open(handle_) == false)
	{
		printf("open db service failed\r\n");
		delete server_;
		server_ = NULL;
	}

	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void Cwin_dbserviceDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void Cwin_dbserviceDlg::OnPaint() 
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
HCURSOR Cwin_dbserviceDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 添加数据过程
void Cwin_dbserviceDlg::OnBnClickedAddData()
{
	// TODO: 在此添加控件通知处理程序代码

	if (server_ == NULL)
	{
		MessageBox("db not opened yet!");
		return;
	}

	acl::string sql;
	myquery* query;

	for (int i = 0; i < 1000; i++)
	{
		query = new myquery(i);
		sql.format("insert into group_tbl('group_name', 'uvip_tbl')"
			" values('中国人-%d', 'test')", i);
		server_->sql_update(sql.c_str(), query);
	}
}

// 查询数据过程
void Cwin_dbserviceDlg::OnBnClickedGetData()
{
	// TODO: 在此添加控件通知处理程序代码

	if (server_ == NULL)
	{
		MessageBox("db not opened yet!");
		return;
	}

	acl::string sql;
	myquery* query;

	for (int i = 0; i < 1000; i++)
	{
		query = new myquery(i);
		sql.format("select * from group_tbl"
			" where group_name='中国人-%d'"
			" and uvip_tbl='test'", i);
		server_->sql_select(sql.c_str(), query);
	}
}

// 删除数据过程
void Cwin_dbserviceDlg::OnBnClickedDeleteData()
{
	// TODO: 在此添加控件通知处理程序代码

	if (server_ == NULL)
	{
		MessageBox("db not opened yet!");
		return;
	}

	acl::string sql;
	myquery* query;

	for (int i = 0; i < 1000; i++)
	{
		query = new myquery(i);
		sql.format("delete from group_tbl"
			" where group_name='中国人-%d'"
			" and uvip_tbl='test'", i);
		server_->sql_update(sql.c_str(), query);
	}
}
