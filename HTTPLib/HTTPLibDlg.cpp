
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
TCHAR LOGFILTER[] = _T("CHTTPLibDlg");
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
	, m_pFileSignMgr(ioThreadPool_.io_service())
	, m_strURL(_T("http://www.baidu.com/"))
	, m_strCookie(_T(""))
	, m_bBaidu(FALSE)
	, m_bQQ(FALSE)
	, m_bDownFile(TRUE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CHTTPLibDlg::~CHTTPLibDlg()
{
	nwThreadPool_.stop();
	ioThreadPool_.stop();
}
void CHTTPLibDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_URL, m_strURL);
	DDX_Text(pDX, IDC_URL2, m_strCookie);
	DDX_Check(pDX, IDC_BAIDU, m_bBaidu);
	DDX_Check(pDX, IDC_QQ, m_bQQ);
	DDX_Check(pDX, IDC_DOWNFILE, m_bDownFile);
}

BEGIN_MESSAGE_MAP(CHTTPLibDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_DOWNLOAD, &CHTTPLibDlg::OnBnClickedDownload)
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
		assert(respond.error.code == cpr::ErrorCode::OK);
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
		assert(respond.error.code == cpr::ErrorCode::OK);
		LogFinal(LOGFILTER, _T("End"));
	},
		CHttpRequest::OnDataRecv(),
		[=](data::byte const * data, size_t size)
	{
		std::ofstream ofs(strFileQQ.c_str(), ios::app);
		ofs.write(data, size);
	}
	);
}

void CHTTPLibDlg::DownloadFile()
{
	static int s_count = 0;
	++s_count;
	std::ostringstream oss;
	oss << s_count;
	string strFileQQ = "d:\\data" + oss.str() + ".zip";
	std::ofstream ofs(strFileQQ.c_str(), ios::app);
	auto pDownloadFile = new CHttpRequest(&hSession_);
	//pDownloadFile->setRange(0, 100);
	if (!m_strCookie.IsEmpty())
	{
		pDownloadFile->setCookie(std::string(CW2A(m_strCookie)));
	}
	pDownloadFile->get(
		std::string(CW2A(m_strURL)), cpr::Parameters{}, CHttpRequest::RecvData_Body | CHttpRequest::RecvData_Header,
		[=](cpr::Response const & respond, data::BufferPtr const & body)
	{
		assert(respond.error.code == cpr::ErrorCode::OK);
		LogFinal(LOGFILTER, _T("End"));
	},
		CHttpRequest::OnDataRecv(),
		[=](data::byte const * data, size_t size)
	{
		data::BufferPtr pData = std::make_shared<data::Buffer>(data, data + size);
		ioThreadPool_.postTask([=]()
		{
			std::ofstream ofs(strFileQQ.c_str(), ios::app);
			ofs.write(pData->data(), pData->length());
		});		
	}
	);
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
