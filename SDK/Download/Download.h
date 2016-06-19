#pragma once
#include <http/HttpRequest.h>
#include <Thread/ThreadPool.h>
#include <http/HttpSession.h>
#include <memory>
#include <fstream>

class CHttpDownload
{
public:
	CHttpDownload(ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession);
	~CHttpDownload();
	void BeginDownload(size_t nThread,std::string const & strSavePath, std::string const & strUrl, std::string const &strCookie = std::string(),::string const & strSHA = std::string(), int64_t fileSize = 0);
private:
	void OnRespond(cpr::Response const & response, data::BufferPtr const & body, int64_t const beg, int64_t end);
	void OnDataRecv(data::byte const * data, size_t size, data::BufferPtr &pData, int64_t & offset);
	void OnSaveData(data::BufferPtr & pData, int64_t offset);
private:
	ThreadPool  & ioThreadPool_;
	ThreadPool  & nwThreadPool_;
	CHttpSession& hSession_;
	std::string strSavePath_;
	std::string strUrl_;
	std::string strCookie_;	
	std::string strSHA_;
	int64_t fileSize_;
	int64_t nextOffset_ = 0;
	std::ofstream ofs_;
};

typedef std::shared_ptr<CHttpDownload> CHttpDownloadPtr;