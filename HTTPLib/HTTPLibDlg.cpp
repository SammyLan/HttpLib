
// HTTPLibDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HTTPLib.h"
#include "HTTPLibDlg.h"
#include "afxdialogex.h"
#include <fstream>
#include <sstream>
#include <cpr/util.h>
#include <boost/filesystem.hpp> 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
static TCHAR LOGFILTER[] = _T("CHTTPLibDlg");
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


// CHTTPLibDlg 对话框



CHTTPLibDlg::CHTTPLibDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_HTTPLIB_DIALOG, pParent)
	, nwThreadPool_(1,"NetworkThread")
	, ioThreadPool_(1,"IOThread")
	, connMgr_(nwThreadPool_.io_service())
	, hSession_(nwThreadPool_.io_service(), &connMgr_)
	, downloadMgr_(ioThreadPool_,nwThreadPool_,hSession_, eventNotifyMgr_)
	, m_pFileSignMgr(ioThreadPool_.io_service())
	, m_strURL(_T(""))
	, m_strCookie(_T(""))
	, m_bBaidu(FALSE)
	, m_bQQ(FALSE)
	, m_bDownFile(TRUE)
	, m_uConn(5)
	, m_strProgress(_T(""))
	, m_strSpeed(_T(""))
	, m_uCurConn(0)
	, m_uPipeline(0)
	, m_strSHA(_T(""))
	, m_uFileSize(0)
	, m_strDir(_T("D:\\download"))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strURL = _T("http://101.227.143.18:80/ftn_handler/6402ad3a3f1742890c7615695531da71646a757bb5ae82b0b2c7f4b2525937aa731df8777fa25fbe81c0bd49f43d65c27f5dc57c3ffd13e8938fb7ecbd9baf02/%E5%A4%AA%E5%AD%90%E5%A6%83%E5%8D%87%E8%81%8C%E8%AE%B0.EP35.2015.HD720P.X264.AAC.Mandarin.CHS.mp4?fname=%E5%A4%AA%E5%AD%90%E5%A6%83%E5%8D%87%E8%81%8C%E8%AE%B0.EP35.2015.HD720P.X264.AAC.Mandarin.CHS.mp4&from=30235&version=3.5.0.1700&uin=240201454");
	m_strCookie = _T("FTN5K=d6bdeb3a");
	m_strSHA = _T("a107ef301247cfabf881f582201a4da4d6bdeb3a");
	m_uFileSize = 782070112;
}

CHTTPLibDlg::~CHTTPLibDlg()
{
	nwThreadPool_.stop();
	ioThreadPool_.stop();
	//eventNotifyMgr_
}
void CHTTPLibDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_URL, m_strURL);
	DDX_Text(pDX, IDC_URL2, m_strCookie);
	DDX_Check(pDX, IDC_BAIDU, m_bBaidu);
	DDX_Check(pDX, IDC_QQ, m_bQQ);
	DDX_Check(pDX, IDC_DOWNFILE, m_bDownFile);
	DDX_Text(pDX, IDC_CONN, m_uConn);
	DDV_MinMaxUInt(pDX, m_uConn, 1, 10);
	DDX_Text(pDX, IDC_EDT_PROGRESS, m_strProgress);
	DDX_Text(pDX, IDC_EDT_SPEED, m_strSpeed);
	DDX_Text(pDX, IDC_EDT_CONN, m_uCurConn);
	DDX_Control(pDX, IDC_BAIDU, m_cBaidu);
	DDX_Control(pDX, IDC_QQ, m_cQQ);
	DDX_Control(pDX, IDC_DOWNFILE, m_cFile);
	DDX_Control(pDX, IDC_STATIC_OPT, m_sOpt);
	DDX_Text(pDX, IDC_EDT_PIPELINE, m_uPipeline);
	DDX_Text(pDX, IDC_SHA, m_strSHA);
	DDX_Text(pDX, IDC_FILESIZE, m_uFileSize);
	DDX_Text(pDX, IDC_DIR, m_strDir);
}

BEGIN_MESSAGE_MAP(CHTTPLibDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_DOWNLOAD, &CHTTPLibDlg::OnBnClickedDownload)
	ON_BN_CLICKED(IDCANCELDOWNLOAD, &CHTTPLibDlg::OnBnClickedCancelDownload)
END_MESSAGE_MAP()


// CHTTPLibDlg 消息处理程序

BOOL CHTTPLibDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CWYString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
#ifdef _DEBUG
	m_cBaidu.ShowWindow(SW_SHOW);
	m_cQQ.ShowWindow(SW_SHOW);
	m_cFile.ShowWindow(SW_SHOW);
	m_sOpt.ShowWindow(SW_SHOW);
#endif
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CHTTPLibDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CHTTPLibDlg::OnPaint()
{
	if (IsIconic())
	{
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
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CHTTPLibDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CHTTPLibDlg::DownloadBaidu()
{
	static int s_count = 0;
	++s_count;
	std::ostringstream oss;
	oss << s_count;
	string strFileBaidu = "d:\\baidu" + oss.str() + ".html";
	auto pRequestBaidu = new CHttpRequest(&hSession_);
	pRequestBaidu->get(std::string(CW2A(m_strURL)), cpr::Parameters{}, CHttpRequest::RecvData_Body | CHttpRequest::RecvData_Header,
		[=](cpr::Response const & respond, data::BufferPtr const & body)
	{
		WYASSERT(respond.error.code == cpr::ErrorCode::OK);
		if (respond.error.code == cpr::ErrorCode::OK)
		{
			if (body.get() != nullptr)
			{
				std::ofstream ofs(strFileBaidu.c_str());
				ofs.write(body->data(), body->length());
			}
		}
	}
	);

}

void CHTTPLibDlg::DownloadQQ()
{
	static int s_count = 0;
	++s_count;
	std::ostringstream oss;
	oss << s_count;
	string strFileQQ = "d:\\qq" + oss.str() + ".html";
	std::ofstream ofs(strFileQQ.c_str(), ios::app);
	auto pRequestQQ = new CHttpRequest(&hSession_);
	pRequestQQ->get(
		std::string("www.qq.com"), cpr::Parameters{}, CHttpRequest::RecvData_Body | CHttpRequest::RecvData_Header,
		[=](cpr::Response const & respond, data::BufferPtr const & body)
	{
		WYASSERT(respond.error.code == cpr::ErrorCode::OK);
		LogFinal(LOGFILTER, _T("End"));
	},
		[=](data::byte const * data, size_t size)
	{
		std::ofstream ofs(strFileQQ.c_str(), ios::app);
		ofs.write(data, size);
	}
	);
}

void CHTTPLibDlg::DownloadFile()
{
	if (m_uPipeline > 1)
	{
		//hSession_.enablePipeline(m_uPipeline);
	}
	static int s_count = 0;
	++s_count;
	std::wostringstream oss;
	oss << s_count;
	wstring strFile = std::wstring(m_strDir) + _T("\\") + oss.str() + _T(".mp4");
	boost::filesystem::path dir(m_strDir);
	if (!boost::filesystem::exists(dir))
	{
		boost::filesystem::create_directory(dir);
	}

	auto taskID = downloadMgr_.AddTask(this,m_uConn, strFile, std::string(CW2A(m_strURL)), std::string(CW2A(m_strCookie)),std::string(CW2A(m_strSHA)),m_uFileSize);
	taskList_.push(taskID);
}

void CHTTPLibDlg::OnBnClickedDownload()
{
	UpdateData(TRUE);
	if (m_bDownFile)
	{
		DownloadFile();
	}
	if (m_bQQ)
	{
		DownloadQQ();
	}
	if (m_bBaidu)
	{
		DownloadBaidu();
	}
}


void CHTTPLibDlg::OnBnClickedCancelDownload()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!taskList_.empty())
	{
		downloadMgr_.RemoveTask(taskList_.top());
		taskList_.pop();
	}
}

void CHTTPLibDlg::OnProgress(WY::TaskID taskID, uint64_t totalSize, uint64_t recvSize, size_t speed)
{
	if (totalSize != 0)
	{
		int progress = int(recvSize * 100 / totalSize);
		m_strProgress.Format(_T("%u%%"),progress);
	}
	float fSpeed = (float)speed / 1024;
	if (fSpeed >= 100)
	{
		fSpeed /= 1024;
		m_strSpeed.Format(_T("%.2fM"), fSpeed);
	}
	else
	{
		m_strSpeed.Format(_T("%.2fKB"), fSpeed);
	}
	m_uCurConn = connMgr_.getSockCount();
	UpdateData(FALSE);
}


void CHTTPLibDlg::OnFinish(WY::TaskID taskID, bool bSuccess, CDownloadTask::ResponseInfo const & info)
{
	float fSpeed = (float)std::get<CDownloadTask::DownloadSpeed>(info) / 1024;
	if (fSpeed >= 100)
	{
		fSpeed /= 1024;
		m_strSpeed.Format(_T("%.2fM"), fSpeed);
	}
	else
	{
		m_strSpeed.Format(_T("%.2fKB"), fSpeed);
	}
	m_strProgress.Format(_T("100%%"));
	m_uCurConn = connMgr_.getSockCount();
	UpdateData(FALSE);
}