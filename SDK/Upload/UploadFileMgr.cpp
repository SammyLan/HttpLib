#include "stdafx.h"
#include "UploadFileMgr.h"
#include <http/HttpGlobal.h>

static TCHAR LOGFILTER[] = _T("CUploadFileMgr");

CUploadFileMgr::CUploadFileMgr(ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession, EventNotifyMgr & eventNotifyMgr)
	:ioThreadPool_(ioThread)
	, nwThreadPool_(nwThread)
	, hSession_(hSession)
	, eventNotifyMgr_(eventNotifyMgr)
{
	LogFinal(LOGFILTER, _T("CUploadFileMgr constructor"));
}

CUploadFileMgr::~CUploadFileMgr()
{
	LogFinal(LOGFILTER, _T("~CUploadFileMgr destructor"));
}

WY::TaskID CUploadFileMgr::AddTask(CUploadTask::IDelegate *pDelegate, size_t nThread, std::wstring const & strSavePath, std::string const & strUrl, std::string const &strCookie , std::string const & strSHA , int64_t fileSize )
{
	static WY::TaskID s_nextTaskID = 0;
	WY::TaskID taskID = 0;
	{
		WY::LockGuard guard(csLock_);
		taskID = ++s_nextTaskID;
		auto pTask = std::make_shared<CUploadTask>(taskID,this,ioThreadPool_, nwThreadPool_, hSession_,nThread, strSavePath, strUrl, strCookie, strSHA, fileSize);
		UploadList_.insert(std::make_pair(taskID, std::make_pair(pTask,pDelegate)));
		pTask->BeginUpload();
	}	
	return taskID;
}

bool CUploadFileMgr::RemoveTask(WY::TaskID const taskID)
{
	CUploadTaskPtr pTask;
	{
		WY::LockGuard guard(csLock_);
		auto it = UploadList_.find(taskID);
		if (it != UploadList_.end())
		{
			pTask = it->second.first;
			UploadList_.erase(it);
		}
	}
	
	if (pTask.get() != nullptr)
	{
		pTask->CancelUpload();
	}
	else
	{
		LogErrorEx(LOGFILTER, _T("找不到task:%llu"), taskID);
		WYASSERT(false);
	}
	return pTask.get() != nullptr;
}


#pragma region delegate

void CUploadFileMgr::OnProgress(WY::TaskID taskID, int64_t totalSize, int64_t recvSize, size_t speed)
{
	eventNotifyMgr_.PostEvent(NewTask(&CUploadFileMgr::OnProgressInner, this, taskID, totalSize, recvSize, speed));
}

void CUploadFileMgr::OnFinish(WY::TaskID taskID, bool bSuccess,CUploadTask::ResponseInfo const & info)
{
	eventNotifyMgr_.PostEvent(NewTask(&CUploadFileMgr::OnFinishInner, this, taskID, bSuccess, info));
}

void CUploadFileMgr::OnProgressInner(WY::TaskID taskID, int64_t totalSize, int64_t recvSize, size_t speed)
{
	WY::LockGuard guard(csLock_);
	auto it = UploadList_.find(taskID);
	if (it != UploadList_.end())
	{
		it->second.second->OnProgress(taskID, totalSize,recvSize,speed);
	}
}

void CUploadFileMgr::OnFinishInner(WY::TaskID taskID, bool bSuccess, CUploadTask::ResponseInfo const & info)
{
	WY::LockGuard guard(csLock_);
	auto it = UploadList_.find(taskID);
	if (it != UploadList_.end())
	{
		it->second.second->OnFinish(taskID, bSuccess, info);
		UploadList_.erase(it);
	}
	else
	{
		LogErrorEx(LOGFILTER, _T("找不到 task:%llu"), taskID);
	}
}
#pragma endregion delegate