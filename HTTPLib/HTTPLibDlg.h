
// HTTPLibDlg.h : 头文件
//

#pragma once
#include "http\HttpSession.h"
#include "http\HttpRequest.h"
#include "http\ThreadPool.h"
#include "FileSign\FileSignTask.h"

// CHTTPLibDlg 对话框
class CHTTPLibDlg : public CDialogEx
{
// 构造
public:
	CHTTPLibDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CHTTPLibDlg();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HTTPLIB_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
