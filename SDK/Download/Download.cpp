#include "stdafx.h"
#include "Download.h"
#include <sstream>
#include <http/HttpGlobal.h>

static TCHAR LOGFILTER[] = _T("CHttpDownload");

size_t const s_save_size = 1024 * 64;

CHttpDownload::CHttpDownload(ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession)
	:ioThreadPool_(ioThread)
	,nwThreadPool_(nwThread)
	,hSession_(hSession)
{

}

CHttpDownload::~CHttpDownload()
{

}

void CHttpDownload::BeginDownload(size_t nThread,std::string const & strSavePath, std::string const & strUrl, std::string const & strCookie, std::string const & strSHA, int64_t fileSize)
{
	strSavePath_ = strSavePath;
	strUrl_ = strUrl;
	strCookie_ = strCookie;
	strSHA_ = strSHA;
	fileSize_ = fileSize;
	nextOffset_ = 0;

	ofs_.open(strSavePath.c_str());

	auto pDownloadFile = new CHttpRequest(&hSession_);
	if (!strCookie.empty())
	{
		pDownloadFile->setCookie(strCookie);
	}
	if (nThread > 1)
	{
		pDownloadFile->headRequest(	std::string(strUrl), cpr::Parameters{},
			[=](cpr::Response const & respond, data::BufferPtr const & body)
		{
			assert(respond.error.code == cpr::ErrorCode::OK);
			auto itLen = respond.header.find("Content-Length");

			int64_t len = 0;
			if (itLen != respond.header.end())
			{
				len = atoll(itLen->second.c_str());
			}
			else
			{
				LogErrorEx(LOGFILTER, _T("can't find the value of Content-Length"));
			}
			LogFinal(LOGFILTER, _T("End"));
			delete this;;
		});
	}
	else
	{
		data::BufferPtr pData = std::make_shared<data::Buffer>();
		pDownloadFile->get(std::string(strUrl), cpr::Parameters{},CHttpRequest::RecvData_Body | CHttpRequest::RecvData_Header, 
			[=](cpr::Response const & respond, data::BufferPtr const & body)
		{
			assert(respond.error.code == cpr::ErrorCode::OK);
			auto itLen = respond.header.find("Content-Length");

			if (itLen != respond.header.end())
			{

			}
			LogFinal(LOGFILTER, _T("End"));
			delete this;
		}, 
			std::bind(&CHttpDownload::OnDataRecv,this,std::placeholders::_1,std::placeholders::_2,pData, nextOffset_)
			);
	}
}

void CHttpDownload::OnRespond(cpr::Response const & response, data::BufferPtr const & body, int64_t const beg, int64_t end)
{

}

void CHttpDownload::OnDataRecv(data::byte const * data, size_t size, data::BufferPtr &pData, int64_t & offset)
{
	auto curSize = pData->size();
	if (curSize + size >= s_save_size)
	{
		size_t appSize = s_save_size - curSize;
		pData->append(data, appSize);
		data::BufferPtr pTmp = std::make_shared<data::Buffer>(data + appSize, size - appSize);
		pData.swap(pTmp);
		ioThreadPool_.postTask(std::bind(&CHttpDownload::OnSaveData, this, pTmp, offset));
		offset += s_save_size;
	}
	else
	{
		pData->append(data, size);
		if ((pData->size() + offset) == fileSize_)
		{
			auto pTmp = pData;
			pData.reset();
			ioThreadPool_.postTask(std::bind(&CHttpDownload::OnSaveData,this, pTmp, offset));
			offset += pTmp->size();
		}
	}
}

void CHttpDownload::OnSaveData(data::BufferPtr & pData, int64_t offset)
{
	auto size = pData->size();
	assert((size == s_save_size) || (size + offset == fileSize_));
	ofs_.write(pData->data(), size);
}