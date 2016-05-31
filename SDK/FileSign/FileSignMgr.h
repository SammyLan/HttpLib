#pragma once
#include <memory>
#include <map>
#include <WYSDK\IWYUploadSDK.h>
#include <WYSDK\Instance.hpp>
#include <Include\WYTypeDef.h>
#include <boost\asio.hpp>

struct CFileSignInfo :public IFileSignInfo
{
	MEMFUNC_IMPL(ULONGLONG, FileSize);
	MEMFUNC_IMPL(float, Speed);
	MEMFUNC_IMPL(DWORD, ScanTime);
	MEMFUNC_IMPL(UINT32, CRC);	
	MEMFUNC_IMPL_STR(SHA);
	MEMFUNC_IMPL_STR(MD5);
	virtual void Release() const override
	{
		delete this;
	}

	std::vector<std::string> const &  GetShaList() const
	{
		return shaList_;
	}

	std::vector<std::string> shaList_;
};

class CFileSignTask;
typedef std::shared_ptr<CFileSignTask> CFileSignTaskPtr;

class  CWYFileSignMgr:public WYUtil::Instance<CWYFileSignMgr>
{
	friend class CFileSignTask;
	typedef CWYFileSignMgr this_class;
	
	typedef map<WYTASKID,CFileSignTaskPtr> TaskList;
public:
	CWYFileSignMgr(boost::asio::io_service& io_service);
	~CWYFileSignMgr();
public:
	WYTASKID AddTask(CWYString const & strFile, DWORD flag,IFileSignDelegate * delegate);
	BOOL RemoveTask(WYTASKID taskID);
	void Stop();
	void OnBegin(WYTASKID taskID);
	void OnProgress(WYTASKID taskID, ULONGLONG fileSize, ULONGLONG completedSize, float spped);
	void OnFinish(WYTASKID taskID,int iError,DWORD dwTotalTime);
private:
	void TryDoNextTask();
private:
	boost::asio::io_service& m_io_service;
	TaskList	m_oTaskList;
	CWYLock		m_csLock;
	BOOL		m_bStop = FALSE;

};