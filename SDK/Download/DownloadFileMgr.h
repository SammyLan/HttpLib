#pragma once
#include "DownloadTask.h"
#include <Thread/ThreadPool.h>
#include <http/HttpSession.h>
#include <map>
#include <string>
#include <NotifyModel/EventNotifyMgr.h>
class CDownloadFileMgr:public CDownloadTask::IDelegate,boost::noncopyable
{
	typedef std::map<int64_t, std::pair<CDownloadTaskPtr, CDownloadTask::IDelegate*>> DownloadList;
public:
	CDownloadFileMgr(ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession, EventNotifyMgr & eventNotifyMgr);
	~CDownloadFileMgr();
	WY::TaskID AddTask(CDownloadTask::IDelegate *pDelegate, size_t nThread, std::wstring const & strSavePath, std::string const & strUrl, std::string const &strCookie, std::string const & strSHA = std::string(), int64_t fileSize = 0);
	bool RemoveTask(WY::TaskID const taskID);
private:
#pragma region  delegate
	virtual void OnProgress(WY::TaskID taskID, uint64_t totalSize, uint64_t recvSize, size_t speed) override;
	virtual void OnFinish(WY::TaskID taskID, bool bSuccess, CDownloadTask::ResponseInfo const & info) override;
	void OnProgressInner(WY::TaskID taskID, uint64_t totalSize, uint64_t recvSize, size_t speed);
	void OnFinishInner(WY::TaskID taskID, bool bSuccess, CDownloadTask::ResponseInfo const & info);
#pragma endregion delegate
private:
	WY::CWYLock		csLock_;
	ThreadPool  &	ioThreadPool_;
	ThreadPool  &	nwThreadPool_;
	CHttpSession&	hSession_;
	EventNotifyMgr & eventNotifyMgr_;
	DownloadList	downLoadList_;
};