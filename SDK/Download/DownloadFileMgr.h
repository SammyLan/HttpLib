#pragma once
#include "DownloadFile.h"
#include <Thread/ThreadPool.h>
#include <http/HttpSession.h>
#include <map>
#include <string>

class CDownloadFileMgr:public CDownloadTask::IDelegate
{
	typedef std::map<int64_t, CDownloadTaskPtr> DownloadList;
public:
	CDownloadFileMgr(ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession);
	~CDownloadFileMgr();
	WY::TaskID AddTask(size_t nThread, std::wstring const & strSavePath, std::string const & strUrl, std::string const &strCookie, std::string const & strSHA = std::string(), int64_t fileSize = 0);
	bool RemoveTask(WY::TaskID const taskID);
private:
#pragma region  delegate
	virtual void OnFinish(bool bSuccess, WY::TaskID taskID, CDownloadTask::ResponseInfo const & info) override;
#pragma endregion delegate
private:
	WY::CWYLock		csLock_;
	ThreadPool  &	ioThreadPool_;
	ThreadPool  &	nwThreadPool_;
	CHttpSession&	hSession_;
	DownloadList	downLoadList_;
};