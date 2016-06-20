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
	LogFinal(LOGFILTER, _T("~CHttpDownload"));
}

bool CHttpDownload::BeginDownload(size_t nThread,std::wstring const & strSavePath, std::string const & strUrl, std::string const & strCookie, std::string const & strSHA, int64_t fileSize)
{
	strSavePath_ = strSavePath.c_str();
	strUrl_ = strUrl;
	strCookie_ = strCookie;
	strSHA_ = strSHA;
	fileSize_ = fileSize;
	nextOffset_ = 0;

	pSaveFile_ = WY::File::CreateAsioFile(ioThreadPool_.io_service(),strSavePath_.c_str(), GENERIC_WRITE,FILE_SHARE_READ, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED);
	if (pSaveFile_.get() == nullptr)
	{
		return false;
	}



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
		data::SaveDataPtr pData = std::make_shared<data::RecvData>();
		pData->first = nextOffset_;

		pDownloadFile->get(std::string(strUrl), cpr::Parameters{},CHttpRequest::RecvData_Body | CHttpRequest::RecvData_Header, 
			std::bind(&CHttpDownload::OnRespond,this,std::placeholders::_1,std::placeholders::_2,pData,0,0),
			std::bind(&CHttpDownload::OnDataRecv,this,std::placeholders::_1,std::placeholders::_2,pData)
			);
	}
	return true;
}

void CHttpDownload::OnRespond(cpr::Response const & response, data::BufferPtr const & body, data::SaveDataPtr const& pData, int64_t const beg, int64_t end)
{
	assert(response.error.code == cpr::ErrorCode::OK);
	auto itLen = response.header.find("Content-Length");

	if (itLen != response.header.end())
	{
		fileSize_ = atoll(itLen->second.c_str());
		assert(pData->first + pData->second.size() == fileSize_);
		if (pData->first + pData->second.size() != fileSize_)
		{
			LogErrorEx(LOGFILTER, _T("���ݳ��Ȳ���,�ļ�����Ϊ%ll,���ܳ���Ϊ%ll"), pData->first + pData->second.size());
		}
	}
	if (pData->second.empty())
	{
		delete this;
		LogFinal(LOGFILTER, _T("delete this"));
	}
	else
	{
		SaveData(pData,true);
	}
	LogFinal(LOGFILTER, _T("End"));
	
}

void CHttpDownload::OnDataRecv(data::byte const * data, size_t size, data::SaveDataPtr const& pData)
{
	auto & pBuff = pData->second;
	auto curSize = pBuff.size();
	auto newSize = curSize + size;
	if (newSize >= s_save_size)
	{
		size_t appSize = size - (newSize%s_save_size);
		pBuff.append(data, appSize);

		data::SaveDataPtr  pTmp = std::make_shared<data::RecvData>();
		pTmp->first = pData->first;
		pData->first += pBuff.size();
		pTmp->second.swap(pBuff);
		pBuff.assign(data + appSize, size - appSize);
		SaveData(pTmp);
	}
	else
	{
		pBuff.append(data, size);
	}
}

void CHttpDownload::SaveData(data::SaveDataPtr const & pData, bool bDel)
{
	auto & pBuff = pData->second;
	auto size = pBuff.size();
	pSaveFile_->async_write_some_at(pData->first, boost::asio::buffer(pBuff.data(), size),
		std::bind(&CHttpDownload::OnSaveDataHandler,this, pData, bDel,std::placeholders::_1, std::placeholders::_2));
}

void CHttpDownload::OnSaveDataHandler(
	data::SaveDataPtr const & pData,bool bDel,
	const boost::system::error_code& error, // Result of operation.
	std::size_t bytes_transferred           // Number of bytes written.	
	)
{
	assert((bytes_transferred == pData->second.size()) || (pData->first + pData->second.size() == fileSize_));
	if (bDel)
	{
		LogFinal(LOGFILTER, _T("delete this"));
		delete this;
	}
}