#pragma once
#include "DownloadFile.h"
#include <Thread/ThreadPool.h>
#include <http/HttpSession.h>
#include <map>
#include <string>

class CDownloadFileMgr:public CDownloadFile::IDelegate
{
	typedef std::map<int64_t, CDownloadFilePtr> DownloadList;
public:
	CDownloadFileMgr(ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession);
	~CDownloadFileMgr();
	WY::TaskID AddDownload(std::wstring const & strSavePath, std::string const & strUrl, std::string const &strCookie, std::string const & strSHA = std::string(), int64_t fileSize = 0);
private:
#pragma region  delegate
	virtual void OnFinish(WY::TaskID taskID, CDownloadFile::ResponseInfo const & info) override;
#pragma endregion delegate
private:
	WY::CWYLock		csLock_;
	ThreadPool  &	ioThreadPool_;
	ThreadPool  &	nwThreadPool_;
	CHttpSession&	hSession_;
	DownloadList	downLoadList_;
};