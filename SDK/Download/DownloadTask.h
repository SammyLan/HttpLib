#pragma once
#include <http/HttpRequest.h>
#include <Thread/ThreadPool.h>
#include <http/HttpSession.h>
#include <memory>
#include <fstream>
#include <File/AsyncFile.h>

class CDownloadTask;
typedef std::shared_ptr<CDownloadTask> CDownloadTaskPtr;
class CDownloadTask:public std::enable_shared_from_this<CDownloadTask>,public boost::noncopyable
{
	typedef std::map<int64_t, CHttpRequestPtr> RequestList;
public:
	enum ResponseInfoPos
	{
		CurlErrorCode = 0,	//curl_error
		CurlErrorMsg = 1,	//curl_error_msg
		HttpCode = 2,		//http - code
		ContentLength = 3,	//Content - Length		
		UserReturnCode = 4	//User - ReturnCode
	};

	typedef std::tuple<int, std::string, int,int64_t,int> ResponseInfo;
	struct IDelegate
	{
		virtual void OnProgress(WY::TaskID taskID,int64_t totalSize, int64_t recvSize, size_t speed) = 0;
		virtual void OnFinish(WY::TaskID taskID,bool bSuccess, CDownloadTask::ResponseInfo const & info) = 0;
	};
	
	 
public:
	CDownloadTask(WY::TaskID const taskID, CDownloadTask::IDelegate * pDelegate,
		ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession,
		size_t nThread, std::wstring const & strSavePath, std::string const & strUrl,
		std::string const &strCookie, ::string const & strSHA, int64_t fileSize);
	~CDownloadTask();
	bool BeginDownload();
	void CancelDownload();
private:
	void GetFileInfo();
	void DownloadFile();
	void OnRespond(cpr::Response const & response, data::BufferPtr const & body, data::SaveDataPtr const& pData,int64_t offset, int64_t fileSize);
	void OnDataRecv(data::byte const * data, size_t size, data::SaveDataPtr const & pData);
	void SaveData(data::SaveDataPtr const & pData,bool bDel = false);
	void OnDataSaveHandler(data::SaveDataPtr const & pData,bool bDel,
		const boost::system::error_code& error, // Result of operation.
		std::size_t bytes_transferred ,          // Number of bytes written.
		std::size_t nBlockIndex
		);
	
	void OnFinish(bool bSuccess);
	ResponseInfo GetResponseInfo(cpr::Response const & response);
	bool DownLoadNextRange(int64_t const beg, int64_t const end);	
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
	std::wstring		strSavePath_;
	std::string	const	strUrl_;
	std::string	const	strCookie_;	
	std::string	const	strSHA_;
	int64_t				fileSize_ = 0;
	int64_t				preRecvSize = 0;
	int64_t				recvSize = 0;
	int64_t				lastReportTime = 0;
	size_t				reportInterval = 1200;//1.2s
	size_t				nThread_ = 1;
	RequestList			requestList_;
	WY::File::AsioFilePtr pSaveFile_;
	WY::CWYLock			csLock_;
	ResponseInfo		responseInfo_;
	bool				bCancel_ = false;
};