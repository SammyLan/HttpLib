
// HTTPLibDlg.h : ͷ�ļ�
//

#pragma once
#include "http\HttpConnMgr.h"
#include "http\HttpSession.h"
#include "http\HttpRequest.h"
#include "Thread\ThreadPool.h"
#include "FileSign\FileSignMgr.h"
#include <Download/DownloadFileMgr.h>
#include <stack>
#include "afxwin.h"

// CHTTPLibDlg �Ի���
class CHTTPLibDlg : public CDialogEx,public CDownloadTask::IDelegate
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
	void DownloadBaidu();
	void DownloadQQ();
	void DownloadFile();
private:
	virtual void OnProgress(WY::TaskID taskID, uint64_t totalSize, uint64_t recvSize, size_t speed) override;
	virtual void OnFinish(WY::TaskID taskID, bool bSuccess, CDownloadTask::ResponseInfo const & info) override;
private:
	ThreadPool nwThreadPool_;
	ThreadPool ioThreadPool_;
	http::CurGlobalInit init_;
	EventNotifyMgr eventNotifyMgr_;
	CHttpConnMgr connMgr_;
	CHttpSession hSession_;
	CDownloadFileMgr downloadMgr_;
	CWYFileSignMgr m_pFileSignMgr;
public:
	afx_msg void OnBnClickedDownload();
	stack<WY::TaskID> taskList_;
	CString m_strURL;
	CString m_strCookie;
	BOOL m_bBaidu;
	BOOL m_bQQ;
	BOOL m_bDownFile;
	UINT m_uConn;
	afx_msg void OnBnClickedCancelDownload();
	CString m_strProgress;
	CString m_strSpeed;
	UINT m_uCurConn;
	CButton m_cBaidu;
	CButton m_cQQ;
	CButton m_cFile;
	CStatic m_sOpt;
	UINT m_uPipeline;
	CString m_strSHA;
	ULONGLONG m_uFileSize;
	CString m_strDir;
};
