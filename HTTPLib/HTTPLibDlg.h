
// HTTPLibDlg.h : ͷ�ļ�
//

#pragma once
#include "http\HttpSession.h"
#include "http\HttpRequest.h"
#include "http\ThreadPool.h"
#include "FileSign\FileSignTask.h"

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
public:
	afx_msg void OnBnClickedOk();
private:
	ThreadPool nwThreadPool_;
	ThreadPool ioThreadPool_;
	CHttpSession hSession_;
	CFileSignTaskPtr m_pFileSignTask;
};
