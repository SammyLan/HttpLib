#include "stdafx.h"
#include "DownloadFileMgr.h"
#include <http/HttpGlobal.h>

static TCHAR LOGFILTER[] = _T("CDownloadFileMgr");

CDownloadFileMgr::CDownloadFileMgr(ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession)
	:ioThreadPool_(ioThread)
	, nwThreadPool_(nwThread)
	, hSession_(hSession)
{

}

CDownloadFileMgr::~CDownloadFileMgr()
{

}

WY::TaskID CDownloadFileMgr::AddDownload(std::wstring const & strSavePath, std::string const & strUrl, std::string const &strCookie , std::string const & strSHA , int64_t fileSize )
{
	static WY::TaskID s_nextTaskID = 0;
	WY::TaskID taskID = 0;
	{
		WY::LockGuard guard(csLock_);
		auto taskID = ++s_nextTaskID;
		auto pTask = std::make_shared<CDownloadFile>(taskID,this,ioThreadPool_, nwThreadPool_, hSession_);
		downLoadList_.insert(std::make_pair(taskID, pTask));
		pTask->BeginDownload(1, strSavePath, strUrl, strCookie, strSHA, fileSize);
	}	
	return taskID;
}

#pragma region delegate

void CDownloadFileMgr::OnFinish(WY::TaskID taskID, int iError, std::string const & strErr)
{
	WY::LockGuard guard(csLock_);
	auto it = downLoadList_.find(taskID);
	if (it != downLoadList_.end())
	{
		downLoadList_.erase(it);
	}
	else
	{
		LogErrorEx(LOGFILTER, _T("�Ҳ��� taskID:%llu"), taskID);
	}
}

#pragma endregion delegate