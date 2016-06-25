#include "stdafx.h"
#include "DownloadFileMgr.h"
#include <http/HttpGlobal.h>

static TCHAR LOGFILTER[] = _T("CDownloadFileMgr");

CDownloadFileMgr::CDownloadFileMgr(ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession, EventNotifyMgr & eventNotifyMgr)
	:ioThreadPool_(ioThread)
	, nwThreadPool_(nwThread)
	, hSession_(hSession)
	, eventNotifyMgr_(eventNotifyMgr)
{
	LogFinal(LOGFILTER, _T("CDownloadFileMgr constructor"));
}

CDownloadFileMgr::~CDownloadFileMgr()
{
	LogFinal(LOGFILTER, _T("~CDownloadFileMgr destructor"));
}

WY::TaskID CDownloadFileMgr::AddTask(CDownloadTask::IDelegate *pDelegate, size_t nThread, std::wstring const & strSavePath, std::string const & strUrl, std::string const &strCookie , std::string const & strSHA , int64_t fileSize )
{
	static WY::TaskID s_nextTaskID = 0;
	WY::TaskID taskID = 0;
	{
		WY::LockGuard guard(csLock_);
		taskID = ++s_nextTaskID;
		auto pTask = std::make_shared<CDownloadTask>(taskID,this,ioThreadPool_, nwThreadPool_, hSession_,nThread, strSavePath, strUrl, strCookie, strSHA, fileSize);
		downLoadList_.insert(std::make_pair(taskID, std::make_pair(pTask,pDelegate)));
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
			pTask = it->second.first;
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

void CDownloadFileMgr::OnProgress(WY::TaskID taskID, int64_t totalSize, int64_t recvSize, size_t speed)
{
	eventNotifyMgr_.PostEvent(NewTask(&CDownloadFileMgr::OnProgressInner, this, taskID, totalSize, recvSize, speed));
}

void CDownloadFileMgr::OnFinish(WY::TaskID taskID, bool bSuccess,CDownloadTask::ResponseInfo const & info)
{
	eventNotifyMgr_.PostEvent(NewTask(&CDownloadFileMgr::OnFinishInner, this, taskID, bSuccess, info));
}

void CDownloadFileMgr::OnProgressInner(WY::TaskID taskID, int64_t totalSize, int64_t recvSize, size_t speed)
{
	WY::LockGuard guard(csLock_);
	auto it = downLoadList_.find(taskID);
	if (it != downLoadList_.end())
	{
		it->second.second->OnProgress(taskID, totalSize,recvSize,speed);
	}
}

void CDownloadFileMgr::OnFinishInner(WY::TaskID taskID, bool bSuccess, CDownloadTask::ResponseInfo const & info)
{
	WY::LockGuard guard(csLock_);
	auto it = downLoadList_.find(taskID);
	if (it != downLoadList_.end())
	{
		it->second.second->OnFinish(taskID, bSuccess, info);
		downLoadList_.erase(it);
	}
	else
	{
		LogErrorEx(LOGFILTER, _T("找不到 task:%llu"), taskID);
	}
}
#pragma endregion delegate