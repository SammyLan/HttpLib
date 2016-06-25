
// HTTPLibDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HTTPLib.h"
#include "HTTPLibDlg.h"
#include "afxdialogex.h"
#include <fstream>
#include <sstream>
#include <cpr/util.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���
static TCHAR LOGFILTER[] = _T("CHTTPLibDlg");
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CHTTPLibDlg �Ի���



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
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strURL = _T("http://sh.yun.ftn.qq.com:80/ftn_handler/0d355f81c2087136ab208c5d8fa58d509530dcf8df48177f6e948547123036dc0deaad223fe3eafb640d7ecaad93133256a846bcb411870b1884fdac023f0598/?fname=%E5%A4%AA%E5%AD%90%E5%A6%83%E5%8D%87%E8%81%8C%E8%AE%B0.EP35.2015.HD720P.X264.AAC.Mandarin.CHS.mp4&from=30322&version=3.5.0.1700&uin=240201454");
	m_strCookie = _T("FTN5K=d6bdeb3a");
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
}

BEGIN_MESSAGE_MAP(CHTTPLibDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_DOWNLOAD, &CHTTPLibDlg::OnBnClickedDownload)
	ON_BN_CLICKED(IDCANCELDOWNLOAD, &CHTTPLibDlg::OnBnClickedCancelDownload)
END_MESSAGE_MAP()


// CHTTPLibDlg ��Ϣ�������

BOOL CHTTPLibDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
#ifdef _DEBUG
	m_cBaidu.ShowWindow(SW_SHOW);
	m_cQQ.ShowWindow(SW_SHOW);
	m_cFile.ShowWindow(SW_SHOW);
	m_sOpt.ShowWindow(SW_SHOW);
#endif
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CHTTPLibDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
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
#ifdef _DEBUG
	wstring strFile = _T("d:\\download\\data") + oss.str() + _T(".mp4");
#else
	wstring strFile = _T("d:\\data") + oss.str() + _T(".mp4");
#endif // _DEBUG
	auto taskID = downloadMgr_.AddTask(this,m_uConn, strFile, std::string(CW2A(m_strURL)), std::string(CW2A(m_strCookie)));
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if (!taskList_.empty())
	{
		downloadMgr_.RemoveTask(taskList_.top());
		taskList_.pop();
	}
}

void CHTTPLibDlg::OnProgress(WY::TaskID taskID, int64_t totalSize, int64_t recvSize, size_t speed)
{
	if (totalSize != 0)
	{
		int progress = int(recvSize * 100 / totalSize);
		m_strProgress.Format(_T("%u"),progress);
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

}