#pragma once
#include <http/HttpRequest.h>
#include <Thread/ThreadPool.h>
#include <http/HttpSession.h>
#include <memory>
#include <fstream>
#include <File/AsyncFile.h>


class CDownloadFile
{
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
		virtual void OnFinish(bool bSuccess, WY::TaskID taskID, ResponseInfo const & info) = 0;
	};
	
	 
public:
	CDownloadFile(WY::TaskID const taskID, CDownloadFile::IDelegate * pDelegate,
		ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession);
	~CDownloadFile();
	bool BeginDownload(size_t nThread,std::wstring const & strSavePath, std::string const & strUrl, std::string const &strCookie = std::string(),::string const & strSHA = std::string(), int64_t fileSize = 0);
private:
	void OnRespond(cpr::Response const & response, data::BufferPtr const & body, data::SaveDataPtr const& pData);
	void OnDataRecv(data::byte const * data, size_t size, data::SaveDataPtr const & pData);
	void SaveData(data::SaveDataPtr const & pData,bool bDel = false);
	void OnSaveDataHandler(data::SaveDataPtr const & pData,bool bDel,
		const boost::system::error_code& error, // Result of operation.
		std::size_t bytes_transferred           // Number of bytes written.
		);
	
	void OnFinish(bool bSuccess,ResponseInfo const & info);
	ResponseInfo GetResponseInfo(cpr::Response const & response);
	void DownLoadNextRange();
private:
	WY::TaskID const	taskID_;
	IDelegate *			pDelegate_;
	ThreadPool  &		ioThreadPool_;
	ThreadPool  &		nwThreadPool_;
	CHttpSession&		hSession_;
	std::wstring		strSavePath_;
	std::string			strUrl_;
	std::string			strCookie_;	
	std::string			strSHA_;
	int64_t				fileSize_;
	size_t				nThread_ = 1;
	CHttpRequestPtr		pHttpRequest_;
	WY::File::AsioFilePtr pSaveFile_;
	int64_t				nextOffset_ = 0;
	WY::CWYLock			csLock_;
};

typedef std::shared_ptr<CDownloadFile> CDownloadFilePtr;