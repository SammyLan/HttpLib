#include "stdafx.h"
#include "DownloadFile.h"
#include <sstream>
#include <http/HttpGlobal.h>
#ifdef min
#undef min
#endif // min

static TCHAR LOGFILTER[] = _T("CHttpDownload");

size_t const s_save_size = 1024 * 64;
size_t const s_block_size = 1024 * 1024 * 1; //10M
size_t const s_srv_block_size = 1024 * 512;

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
	nThread_ = nThread;

	pSaveFile_ = WY::File::CreateAsioFile(ioThreadPool_.io_service(),strSavePath_.c_str(), GENERIC_WRITE,FILE_SHARE_READ, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED);
	if (pSaveFile_.get() == nullptr)
	{
		ResponseInfo resp;
		std::get<UserReturnCode>(resp) = ::GetLastError();
		OnFinish(false, resp);
		return false;
	}

	auto pHttpRequest = std::make_shared<CHttpRequest>(&hSession_);
	requestList_.insert(std::make_pair(0, pHttpRequest));
	if (!strCookie.empty())
	{
		pHttpRequest->setCookie(strCookie);
	}
	if (nThread > 1)
	{
		int64_t beg = WYTime::g_watch.Now();
		pHttpRequest->headRequest(	std::string(strUrl), cpr::Parameters{},
			[=](cpr::Response const & response, data::BufferPtr const & body)
		{
			int64_t end = WYTime::g_watch.Now();
			LogFinal(LOGFILTER, _T("take time=%llu"), end - beg);
			auto pRequest = *requestList_.begin();
			requestList_.clear();
			auto && ret = GetResponseInfo(response);
			auto curlCode = std::get<CurlErrorCode>(ret);
			auto httpCode = std::get<HttpCode>(ret);
			auto userReturnCode = std::get<UserReturnCode>(ret);
			bool bSuccess = ((curlCode == (int)cpr::ErrorCode::OK) && (httpCode == 200) && (userReturnCode == 0));
			if (!bSuccess)
			{
				assert(false);
				OnFinish(bSuccess,ret);
			}
			else
			{
				if (fileSize_ == 0)
				{
					fileSize_ = std::get<ContentLength>(ret);
				}
				else
				{
					assert(fileSize_ == std::get<ContentLength>(ret));
					std::get<UserReturnCode>(ret) = 1;//TODO:
					OnFinish(false, ret);
					return;
				}
				size_t nThread =(int) std::min((int64_t)nThread_, fileSize_/s_block_size);
				if (nThread == 0)
				{
					nThread = 1;
				}
				LockGuard gurad(csLock_);
				int64_t beg = 0;
				int64_t block_size = fileSize_ / nThread;
				std::vector<int64_t> blockInfo;
				blockInfo.push_back(beg);
				beg += block_size;
				while (beg < fileSize_)
				{
					beg = (beg + s_srv_block_size - 1) / s_srv_block_size * s_srv_block_size;
					blockInfo.push_back(beg);
					beg += block_size;
				}
				if (beg > fileSize_)
				{
					beg = fileSize_;
					blockInfo.push_back(beg);
				}
				assert((nThread + 1) == blockInfo.size());
				for (size_t i = 1; i < blockInfo.size(); ++i)
				{
					DownLoadNextRange(blockInfo[i-1], blockInfo[i]);
				}
			}
		});
	}
	else
	{
		data::SaveDataPtr pData = std::make_shared<data::RecvData>();
		pData->first = 0;

		pHttpRequest->get(std::string(strUrl), cpr::Parameters{},CHttpRequest::RecvData_Body | CHttpRequest::RecvData_Header,
			std::bind(&CDownloadFile::OnRespond,this,std::placeholders::_1,std::placeholders::_2,pData),
			std::bind(&CDownloadFile::OnDataRecv,this,std::placeholders::_1,std::placeholders::_2,pData)
			);
	}
	return true;
}

void CDownloadFile::OnRespond(cpr::Response const & response, data::BufferPtr const & body, data::SaveDataPtr const& pData)
{
	requestList_.clear();
	auto &&ret = GetResponseInfo(response);
	auto curlCode = std::get<CurlErrorCode>(ret);
	auto httpCode = std::get<HttpCode>(ret);
	auto userReturnCode = std::get<UserReturnCode>(ret);

	bool bSuccess = ( (curlCode == (int)cpr::ErrorCode::OK) && (httpCode == 200) && (userReturnCode == 0));
	if (bSuccess)
	{
		if (fileSize_ == 0)
		{
			fileSize_ = std::get<ContentLength>(ret);
		}
		else
		{
			assert(fileSize_ == std::get<ContentLength>(ret));
		}
		
		if (pData->first + pData->second.size() != fileSize_)
		{
			assert(false);
			LogErrorEx(LOGFILTER, _T("数据长度不对,文件长度为%ll,接受长度为%ll"), pData->first + pData->second.size());
		}

		if (pData->second.empty())
		{
			OnFinish(bSuccess,ret);
		}
		else
		{
			SaveData(pData, true);
		}
	}
	else
	{
		assert(false);
		OnFinish(bSuccess,ret);
	}
}

void CDownloadFile::OnRespondEx(cpr::Response const & response, data::BufferPtr const & body, data::SaveDataPtr const& pData, int64_t offset)
{
	auto itFind = requestList_.find(offset);
	auto pRequest = itFind->second;
	requestList_.erase(itFind);

	auto &&ret = GetResponseInfo(response);
	auto curlCode = std::get<CurlErrorCode>(ret);
	auto httpCode = std::get<HttpCode>(ret);
	auto userReturnCode = std::get<UserReturnCode>(ret);

	bool bSuccess = ((curlCode == (int)cpr::ErrorCode::OK) && (httpCode == 200 || httpCode == 206) && (userReturnCode == 0));
	if (bSuccess)
	{
		auto recvSize = std::get<ContentLength>(ret);
		if (pData->first + pData->second.size() != offset + recvSize)
		{
			assert(false);
			LogErrorEx(LOGFILTER, _T("数据长度不对,文件长度为%ll,接受长度为%ll"), pData->first + pData->second.size());

		}
		else
		{
			SaveData(pData);
		}
	}
	else
	{
		data::Buffer const & headerStr = pRequest->getHeader();
		LogErrorEx(LOGFILTER, _T("下载出错:%S"), headerStr.data());
		assert(false);
		//TODO:Error
	}
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
	if (bDel)
	{
		ResponseInfo info;
		OnFinish(true,info);
	}
}

void CDownloadFile::OnFinish(bool bSuccess,ResponseInfo const & info)
{
	if (!bSuccess)
	{
		LogErrorEx(LOGFILTER, _T("curl_error=%i, curl_error_msg=%S, HttpCode=%i, UserReturnCode=%i\r\nURL=%S"),
			std::get<CurlErrorCode>(info),
			std::get<CurlErrorMsg>(info).c_str(),
			::get<HttpCode>(info),
			::get<UserReturnCode>(info));
	}
	ioThreadPool_.postTask(std::bind(&IDelegate::OnFinish, pDelegate_,bSuccess, taskID_,info));
}

CDownloadFile::ResponseInfo CDownloadFile::GetResponseInfo(cpr::Response const & response)
{
	ResponseInfo res;
	auto const & header = response.header;
	std::get<HttpCode>(res) = response.status_code;
	std::get<CurlErrorCode>(res) = (int)response.error.code;
	if (response.error.code == cpr::ErrorCode::OK)
	{
		//std::get<CurlErrorMsg>(ret);
		auto itLen = header.find("Content-Length");
		if (itLen != header.end())
		{
			std::get<ContentLength>(res) = atoll(itLen->second.c_str());
		}
		else
		{
			LogErrorEx(LOGFILTER, _T("can't find the value of Content-Length"));
		}

		auto itRet = header.find("User-ReturnCode");
		if (itRet != header.end())
		{
			std::get<UserReturnCode>(res) = atoi(itRet->second.c_str());
		}
		else
		{
			LogErrorEx(LOGFILTER, _T("can't find the value of ser-ReturnCode"));
		}
	}
	else
	{
		std::get<CurlErrorMsg>(res) = response.error.message;
	}
	return res;
}

bool CDownloadFile::DownLoadNextRange(int64_t const beg, int64_t const end)
{
	if (end != fileSize_)
	{
		if ((end & 0x7FFFF) != 0)
		{
			assert(false);
			//TODO:
		}
	}
	LogDev(LOGFILTER, _T("Request next range:%llu"), beg);
	
	auto pHttpRequest = std::make_shared<CHttpRequest>(&hSession_);
	requestList_.insert(std::make_pair(beg, pHttpRequest));

	data::SaveDataPtr pData = std::make_shared<data::RecvData>();
	pData->first = beg;	
	pHttpRequest->setCookie(strCookie_);
	pHttpRequest->setRange(beg, end - 1);
	
	pHttpRequest->get(std::string(strUrl_), cpr::Parameters{}, CHttpRequest::RecvData_Body | CHttpRequest::RecvData_Header,
		std::bind(&CDownloadFile::OnRespondEx, this, std::placeholders::_1, std::placeholders::_2, pData,beg),
		std::bind(&CDownloadFile::OnDataRecv, this, std::placeholders::_1, std::placeholders::_2, pData)
		);
	return true;
}
