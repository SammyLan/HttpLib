#include "stdafx.h"
#include "FileSignMgr.h"
#include "FileSignTask.h"

static TCHAR LOGFILTER[] = _T("CWYFileSignMgr");

CWYFileSignMgr::CWYFileSignMgr(boost::asio::io_service& io_service)
	:m_io_service(io_service)
{

}

CWYFileSignMgr::~CWYFileSignMgr()
{
	LockGuard guard(m_csLock);
	m_oTaskList.clear();
}

WYTASKID CWYFileSignMgr::AddTask(CWYString const & strFile, DWORD flag, IFileSignDelegate * delegate)
{
	WYTASKID taskID = 0;
	{
		LockGuard guard(m_csLock);
		static WYTASKID s_nextTaskID = 0;
		taskID = ++s_nextTaskID;
		m_oTaskList.insert(make_pair(taskID, CFileSignTaskPtr(new CFileSignTask(taskID, strFile,this, delegate))));
	}
	TryDoNextTask();
	return taskID;
}


BOOL CWYFileSignMgr::RemoveTask(WYTASKID taskID)
{
	BOOL bRet = FALSE;
	LockGuard guard(m_csLock);
	auto it = m_oTaskList.find(taskID);
	ATLASSERT(it != m_oTaskList.end());
	if (it != m_oTaskList.end())
	{
		it->second->CancelTask();
		m_oTaskList.erase(it);
		bRet =  TRUE;
	}
	TryDoNextTask();
	return bRet;
}

void CWYFileSignMgr::TryDoNextTask()
{
	LockGuard guard(m_csLock);
	if (m_bStop)
	{
		return ;
	}
	
	auto it = m_oTaskList.begin();
	if (it != m_oTaskList.end() &&!it->second->m_bRunning)
	{
		it->second->BeginTask(m_io_service);
	}
}

void CWYFileSignMgr::Stop()
{
	LockGuard guard(m_csLock);
	m_bStop = TRUE;
	for (auto it = m_oTaskList.begin(); it != m_oTaskList.end(); ++it)
	{
		it->second->CancelTask();
	}
	m_oTaskList.clear();
}

void CWYFileSignMgr::OnBegin(WYTASKID taskID)
{
	LockGuard guard(m_csLock);
	auto it = m_oTaskList.find(taskID);
	ATLASSERT(it != m_oTaskList.end());
	if (it != m_oTaskList.end())
	{
		auto pTask = it->second;
		if (pTask->m_pCallback != NULL)
		{
			pTask->m_pCallback->onFileSignBegin(pTask->m_uFileSize);
		}
	}
	else
	{
		LogErrorEx(LOGFILTER,_T("task±»É¾³ý:%llu"), taskID);
	}
}

void CWYFileSignMgr::OnProgress(WYTASKID taskID, ULONGLONG fileSize, ULONGLONG completedSize, float spped)
{
	LockGuard guard(m_csLock);
	auto it = m_oTaskList.find(taskID);
	ATLASSERT(it != m_oTaskList.end());
	if (it != m_oTaskList.end())
	{
		auto pTask = it->second;
		if (pTask->m_pCallback != NULL)
		{
			pTask->m_pCallback->onFileSignProgress(fileSize, completedSize, spped);
		}
	}
	else
	{
		LogErrorEx(LOGFILTER,_T("task±»É¾³ý:%llu"), taskID);
	}
}

void CWYFileSignMgr::OnFinish(WYTASKID taskID, int iError, DWORD dwTotalTime)
{
	LockGuard guard(m_csLock);
	auto it = m_oTaskList.find(taskID);
	ATLASSERT(it != m_oTaskList.end());
	if (it != m_oTaskList.end())
	{
		auto pTask = it->second;
		if (pTask->m_pCallback != NULL)
		{
			if (dwTotalTime == 0)
			{
				dwTotalTime = 1;
			}
			CFileSignInfo * pInfo = new CFileSignInfo;
			pInfo->FileSize_ = pTask->m_uFileSize;
			pInfo->Speed_ = (float)pTask->m_uFileSize * 1000 / dwTotalTime / 1024 / 1024;
			pInfo->ScanTime_ = dwTotalTime;
			pInfo->CRC_ = pTask->m_validateCRC;
			pInfo->SHA_ = pTask->m_strSHA;
			pInfo->shaList_.swap(pTask->m_oShaList);
			it->second->m_pCallback->onFileSignEnd(iError, 0, pInfo);
		}
		m_oTaskList.erase(it);
	}
	else
	{
		LogErrorEx(LOGFILTER,_T("task±»É¾³ý:%llu"), taskID);
	}
	TryDoNextTask();
}
