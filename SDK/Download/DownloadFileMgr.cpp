#include "stdafx.h"
#include "DownloadFileMgr.h"
#include <http/HttpGlobal.h>

static TCHAR LOGFILTER[] = _T("CDownloadFileMgr");

CDownloadFileMgr::CDownloadFileMgr(ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession)
	:ioThreadPool_(ioThread)
	, nwThreadPool_(nwThread)
	, hSession_(hSession)
{
	LogFinal(LOGFILTER, _T("CDownloadFileMgr constructor"));
}

CDownloadFileMgr::~CDownloadFileMgr()
{
	LogFinal(LOGFILTER, _T("~CDownloadFileMgr destructor"));
}

WY::TaskID CDownloadFileMgr::AddTask(size_t nThread, std::wstring const & strSavePath, std::string const & strUrl, std::string const &strCookie , std::string const & strSHA , int64_t fileSize )
{
	static WY::TaskID s_nextTaskID = 0;
	WY::TaskID taskID = 0;
	{
		WY::LockGuard guard(csLock_);
		taskID = ++s_nextTaskID;
		auto pTask = std::make_shared<CDownloadTask>(taskID,this,ioThreadPool_, nwThreadPool_, hSession_,nThread, strSavePath, strUrl, strCookie, strSHA, fileSize);
		downLoadList_.insert(std::make_pair(taskID, pTask));
		pTask->BeginDownload();
	}	
	return taskID;
}

bool CDownloadFileMgr::RemoveTask(WY::TaskID const taskID)
{
	CDownloadTaskPtr pTask;
	{
		WY::LockGuard guard(csLock_);
		auto it = downLoadList_.find(taskID);
		if (it != downLoadList_.end())
		{
			pTask = it->second;
			downLoadList_.erase(it);
		}
	}
	
	if (pTask.get() != nullptr)
	{
		pTask->CancelDownload();
	}
	else
	{
		LogErrorEx(LOGFILTER, _T("找不到task:%llu"), taskID);
		WYASSERT(false);
	}
	return pTask.get() != nullptr;
}


#pragma region delegate

void CDownloadFileMgr::OnFinish(bool bSuccess,WY::TaskID taskID, CDownloadTask::ResponseInfo const & info)
{
	WY::LockGuard guard(csLock_);
	auto it = downLoadList_.find(taskID);
	if (it != downLoadList_.end())
	{
		auto pTask = it->second;
		downLoadList_.erase(it);
	}
	else
	{
		LogErrorEx(LOGFILTER, _T("找不到 task:%llu"), taskID);
	}
}

#pragma endregion delegate