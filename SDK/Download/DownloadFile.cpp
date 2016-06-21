#include "stdafx.h"
#include "DownloadFile.h"
#include <sstream>
#include <http/HttpGlobal.h>

static TCHAR LOGFILTER[] = _T("CHttpDownload");

size_t const s_save_size = 1024 * 64;

CDownloadFile::CDownloadFile(WY::TaskID const taskID, CDownloadFile::IDelegate * pDelegate,ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession)
	:taskID_(taskID)
	,pDelegate_(pDelegate)
	,ioThreadPool_(ioThread)
	,nwThreadPool_(nwThread)
	,hSession_(hSession)
{

}

CDownloadFile::~CDownloadFile()
{
	LogFinal(LOGFILTER, _T("~CHttpDownload"));
}

bool CDownloadFile::BeginDownload(size_t nThread,std::wstring const & strSavePath, std::string const & strUrl, std::string const & strCookie, std::string const & strSHA, int64_t fileSize)
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
			[=](cpr::Response const & response, data::BufferPtr const & body)
		{
			auto ret = GetResponseInfo(response);
			if (std::get<0>(ret) != 0)
			{
				OnFinish(std::get<0>(ret), std::get<1>(ret));
			}
			else
			{
				//TODO
			}
			LogFinal(LOGFILTER, _T("End"));
		});
	}
	else
	{
		data::SaveDataPtr pData = std::make_shared<data::RecvData>();
		pData->first = nextOffset_;

		pDownloadFile->get(std::string(strUrl), cpr::Parameters{},CHttpRequest::RecvData_Body | CHttpRequest::RecvData_Header, 
			std::bind(&CDownloadFile::OnRespond,this,std::placeholders::_1,std::placeholders::_2,pData,0,0),
			std::bind(&CDownloadFile::OnDataRecv,this,std::placeholders::_1,std::placeholders::_2,pData)
			);
	}
	return true;
}

void CDownloadFile::OnRespond(cpr::Response const & response, data::BufferPtr const & body, data::SaveDataPtr const& pData, int64_t const beg, int64_t end)
{
	assert(response.error.code == cpr::ErrorCode::OK);
	auto itLen = response.header.find("Content-Length");

	if (itLen != response.header.end())
	{
		fileSize_ = atoll(itLen->second.c_str());
		assert(pData->first + pData->second.size() == fileSize_);
		if (pData->first + pData->second.size() != fileSize_)
		{
			LogErrorEx(LOGFILTER, _T("数据长度不对,文件长度为%ll,接受长度为%ll"), pData->first + pData->second.size());
		}
	}
	auto ret = GetResponseInfo(response);
	if (pData->second.empty() || std::get<0>(ret) != 0)
	{
		OnFinish(std::get<0>(ret), std::get<1>(ret));
	}
	else
	{
		SaveData(pData,true);
	}
	LogFinal(LOGFILTER, _T("End"));
	
}

void CDownloadFile::OnDataRecv(data::byte const * data, size_t size, data::SaveDataPtr const& pData)
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

void CDownloadFile::SaveData(data::SaveDataPtr const & pData, bool bDel)
{
	auto & pBuff = pData->second;
	auto size = pBuff.size();
	pSaveFile_->async_write_some_at(pData->first, boost::asio::buffer(pBuff.data(), size),
		std::bind(&CDownloadFile::OnSaveDataHandler,this, pData, bDel,std::placeholders::_1, std::placeholders::_2));
}

void CDownloadFile::OnSaveDataHandler(
	data::SaveDataPtr const & pData,bool bDel,
	const boost::system::error_code& error, // Result of operation.
	std::size_t bytes_transferred           // Number of bytes written.	
	)
{
	assert((bytes_transferred == pData->second.size()) || (pData->first + pData->second.size() == fileSize_));
	if (bDel)
	{
		OnFinish(0, "");
	}
}

void CDownloadFile::OnFinish(int iError, std::string const & strErr)
{
	ioThreadPool_.postTask(std::bind(&IDelegate::OnFinish, pDelegate_, taskID_, iError, strErr));
}

std::tuple<int, std::string, int64_t> CDownloadFile::GetResponseInfo(cpr::Response const & response)
{
	std::tuple<int, std::string, int64_t> ret;

	auto const & header = response.header;

	if (response.error.code == cpr::ErrorCode::OK)
	{
		std::get<0>(ret) = 0;
		auto itLen = header.find("Content-Length");
		if (itLen != header.end())
		{
			std::get<2>(ret) = atoll(itLen->second.c_str());
		}
		else
		{
			LogErrorEx(LOGFILTER, _T("can't find the value of Content-Length"));
		}
	}
	else
	{
		std::get<0>(ret) = (int)response.error.code;
		std::get<1>(ret) = response.error.message;
	}
	return ret;
}
