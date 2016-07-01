#pragma once
#include <http/HttpRequest.h>
#include <Thread/ThreadPool.h>
#include <http/HttpSession.h>
#include <memory>
#include <fstream>
#include <File/WYFile.h>
#include "DownloadInfo.h"

class CDownloadTask;
typedef std::shared_ptr<CDownloadTask> CDownloadTaskPtr;
class CDownloadTask:public std::enable_shared_from_this<CDownloadTask>,public boost::noncopyable
{
	typedef std::map<uint64_t, CHttpRequestPtr> RequestList;
	typedef std::pair<int64_t, data::Buffer> RecvData;	//offset-data
	typedef std::shared_ptr<RecvData> SaveDataPtr;
public:
	enum ResponseInfoPos
	{
		CurlErrorCode = 0,	//curl_error
		CurlErrorMsg = 1,	//curl_error_msg
		HttpCode = 2,		//http - code
		ContentLength = 3,	//Content - Length		
		UserReturnCode = 4,	//User - ReturnCode
		DownloadSpeed = 5	//平均速度
	};

	typedef std::tuple<int, std::string, int, uint64_t,int,size_t> ResponseInfo;
	struct IDelegate
	{
		virtual void OnProgress(WY::TaskID taskID, uint64_t totalSize, uint64_t recvSize, size_t speed) = 0;
		virtual void OnFinish(WY::TaskID taskID,bool bSuccess, CDownloadTask::ResponseInfo const & info) = 0;
	};
	
	 
public:
	CDownloadTask(WY::TaskID const taskID, CDownloadTask::IDelegate * pDelegate,
		ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession,
		size_t nThread, std::wstring const & strSavePath, std::string const & strUrl,
		std::string const &strCookie, ::string const & strSHA, uint64_t fileSize);
	~CDownloadTask();
	bool BeginDownload();
	void CancelDownload();
private:
	void GetFileInfo();
	void DownloadFile();
	void OnRespond(cpr::Response const & response, data::BufferPtr const & body, CDownloadTask::SaveDataPtr const& pData, uint64_t offset, uint64_t fileSize);
	void OnDataRecv(data::byte const * data, size_t size, CDownloadTask::SaveDataPtr const & pData);
	void SaveData(CDownloadTask::SaveDataPtr const & pData,bool bDel = false);
	void OnDataSaveHandler(CDownloadTask::SaveDataPtr const & pData,bool bDel,
		const boost::system::error_code& error, // Result of operation.
		std::size_t bytes_transferred ,          // Number of bytes written.
		std::size_t nBlockIndex
		);
	
	void OnFinish(bool bSuccess);
	ResponseInfo GetResponseInfo(cpr::Response const & response);
	bool DownLoadNextRange(CDownloadInfo::PieceInfo const & info);
	bool CreareFile();
	bool SetFile();
	void Cancel();
#pragma region dump info
	void DumpRespond(cpr::Response const & response);
	void DumpSelfInfo(LPCTSTR strMsg, int logLevel = LOGL_Info);
#pragma endregion dump info
private:
	WY::TaskID const	taskID_;
	IDelegate *			pDelegate_;
	ThreadPool  &		ioThreadPool_;
	ThreadPool  &		nwThreadPool_;
	CHttpSession&		hSession_;
	std::string	const	strUrl_;
	std::string	const	strCookie_;	
	uint64_t			preRecvSize = 0;
	uint64_t			recvSize = 0;
	uint64_t			lastReportTime = 0;
	size_t const		reportInterval = 1200;//1.2s
	size_t				nBeginDownload_ = 0;
	RequestList			requestList_;
	WY::File::AsioFilePtr pSaveFile_;
	CDownloadInfo		descFile_;
	WY::CWYLock			csLock_;
	ResponseInfo		responseInfo_;
	bool				bCancel_ = false;
	
};