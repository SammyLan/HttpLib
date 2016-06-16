
// HTTPLibDlg.h : ͷ�ļ�
//

#pragma once
#include "http\HttpConnMgr.h"
#include "http\HttpSession.h"
#include "http\HttpRequest.h"
#include "Thread\ThreadPool.h"
#include "FileSign\FileSignMgr.h"

// CHTTPLibDlg �Ի���
class CHTTPLibDlg : public CDialogEx
{
// ����
public:
	CHTTPLibDlg(CWnd* pParent = NULL);	// ��׼���캯��
	~CHTTPLibDlg();
// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HTTPLIB_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	ThreadPool nwThreadPool_;
	ThreadPool ioThreadPool_;
	CHttpConnMgr connMgr_;
	CHttpSession hSession_;
	CWYFileSignMgr m_pFileSignMgr;
public:
	afx_msg void OnBnClickedDownload();
	CString m_strURL;
	CString m_strCookie;
};
