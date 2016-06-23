#include "stdafx.h"
#include "DownloadFile.h"
#include <sstream>
#include <http/HttpGlobal.h>
#include <winioctl.h>
#include "WYErrorDef.h"
#ifdef min
#undef min
#endif // min

static TCHAR LOGFILTER[] = _T("CHttpDownload");

size_t const s_save_size = 1024 * 64;
size_t const s_block_size = 1024 * 1024 * 1; //10M
size_t const s_srv_block_size = 1024 * 512;

CDownloadTask::CDownloadTask(WY::TaskID const taskID, CDownloadTask::IDelegate * pDelegate,
	ThreadPool & ioThread, ThreadPool & nwThread, CHttpSession& hSession,
	size_t nThread, std::wstring const & strSavePath, std::string const & strUrl,
	std::string const & strCookie, std::string const & strSHA, int64_t fileSize)
	:taskID_(taskID)
	,pDelegate_(pDelegate)
	,ioThreadPool_(ioThread)
	,nwThreadPool_(nwThread)
	,hSession_(hSession)
	,strSavePath_(strSavePath)
	,strUrl_(strUrl)
	,strCookie_(strCookie)
	,strSHA_(strSHA)
	,fileSize_(fileSize)
	,nThread_(nThread)
{
	std::get<HttpCode>(responseInfo_) = 200;
	DumpSelfInfo(_T("Create download Task"));
}

CDownloadTask::~CDownloadTask()
{
	DumpSelfInfo(_T("Destory download Task"));
}

bool CDownloadTask::BeginDownload()
{
	LockGuard gurad(csLock_);
	if (!CreareFile())
	{
		std::get<UserReturnCode>(responseInfo_) = ::GetLastError();
		OnFinish(false);
		DumpSelfInfo(_T("Create file failed"), LOGL_Error);
		return false;
	}

	if (nThread_ > 1)
	{
		GetFileInfo();
	}
	else
	{
		DownloadFile();
	}
	return true;
}

void CDownloadTask::CancelDownload()
{
	LockGuard gurad(csLock_);
	bCancel_ = true;
	for (auto it = requestList_.begin(); it != requestList_.end(); it++)
	{
		it->second->cancel();
	}
	DumpSelfInfo(_T("Cancel task"));
}

void CDownloadTask::GetFileInfo()
{
	auto pHttpRequest = std::make_shared<CHttpRequest>(&hSession_);
	requestList_.insert(std::make_pair(0, pHttpRequest));
	if (!strCookie_.empty())
	{
		pHttpRequest->setCookie(strCookie_);
	}
	int64_t beg = WYTime::g_watch.Now();
	pHttpRequest->headRequest(std::string(strUrl_), cpr::Parameters{},
		[=](cpr::Response const & response, data::BufferPtr const & body)
	{
		LockGuard gurad(csLock_);
		
		int64_t end = WYTime::g_watch.Now();
		LogFinal(LOGFILTER, _T("take time=%llu"), end - beg);

		auto pRequest = *requestList_.begin();
		requestList_.clear();
		auto && ret = GetResponseInfo(response);
		if (this->bCancel_)
		{
			OnFinish(true);
			return;
		}

		auto curlCode = std::get<CurlErrorCode>(ret);
		auto httpCode = std::get<HttpCode>(ret);
		auto userReturnCode = std::get<UserReturnCode>(ret);
		bool bSuccess = ((curlCode == (int)cpr::ErrorCode::OK) && (httpCode == 200) && (userReturnCode == 0));
		if (bSuccess)
		{
			if (fileSize_ == 0)
			{
				fileSize_ = std::get<ContentLength>(ret);
			}
			else
			{
				if (fileSize_ != std::get<ContentLength>(ret))
				{
					WYASSERT(false);
					std::get<UserReturnCode>(responseInfo_) = WYErrorCode::Download_HeadSizeConflict;
					OnFinish(false);
					return;
				}
			}
			DownloadFile();
		}
		else
		{
			WYASSERT(false);
			responseInfo_ = ret;
			OnFinish(false);
		}
	});
}

void CDownloadTask::DownloadFile()
{
	DumpSelfInfo(_T("Start download"));
	SetFile();
	size_t nThread = (int)std::min((int64_t)nThread_, fileSize_ / s_block_size);
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
	if (beg >= fileSize_)
	{
		beg = fileSize_;
		blockInfo.push_back(beg);
	}
	WYASSERT((nThread + 1) == blockInfo.size());
	for (size_t i = 1; i < blockInfo.size(); ++i)
	{
		DownLoadNextRange(blockInfo[i - 1], blockInfo[i]);
	}
}

void CDownloadTask::OnRespond(cpr::Response const & response, data::BufferPtr const & body, data::SaveDataPtr const& pData, int64_t offset,int64_t fileSize)
{
	LockGuard gurad(csLock_);
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
		do
		{
			auto recvSize = std::get<ContentLength>(ret);
			if (fileSize == 0)//不需要从服务器获取文件大小
			{
				fileSize_ = recvSize;
			}
			WYASSERT(fileSize == recvSize);
			bSuccess = ((fileSize == recvSize) && (pData->first + pData->second.size() == offset + recvSize));
			if (!bSuccess)
			{
				WYASSERT(false);
				LogErrorEx(LOGFILTER, _T("数据长度不对,文件长度为%llu,接受长度为%llu"), pData->first + pData->second.size(), offset + recvSize);
				DumpSelfInfo(_T("请求大小跟接收大小不一致"), LOGL_Error);
				std::get<UserReturnCode>(responseInfo_) = WYErrorCode::Download_RequestSizeConflict;
				break;
			}
			else
			{
				SaveData(pData, requestList_.empty());
			}
		} while (false);		
	}
	else
	{
		responseInfo_ = ret;
		data::Buffer const & headerStr = pRequest->getHeader();
		LogErrorEx(LOGFILTER, _T("下载出错:%S"), headerStr.data());
		WYASSERT(false);
	}
	if (requestList_.empty())
	{
		OnFinish(bSuccess);
	}
}

void CDownloadTask::OnDataRecv(data::byte const * data, size_t size, data::SaveDataPtr const& pData)
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

void CDownloadTask::SaveData(data::SaveDataPtr const & pData, bool bDel)
{
	auto & pBuff = pData->second;
	auto size = pBuff.size();
	pSaveFile_->async_write_some_at(pData->first, boost::asio::buffer(pBuff.data(), size),
		std::bind(&CDownloadTask::OnSaveDataHandler,this, pData, bDel,std::placeholders::_1, std::placeholders::_2,shared_from_this()));
}

void CDownloadTask::OnSaveDataHandler(
	data::SaveDataPtr const & pData,bool bDel,
	const boost::system::error_code& error, // Result of operation.
	std::size_t bytes_transferred,           // Number of bytes written.
	CDownloadTaskPtr const& pTask
	)
{
	if (bDel)
	{
		OnFinish(true);
	}
}

void CDownloadTask::OnFinish(bool bSuccess)
{
	if (bCancel_)
	{
		DumpSelfInfo(_T("OnFinsh: Cancel完成"), LOGL_Error);
		bSuccess = true;
	}
	if (!bSuccess)
	{
		LogErrorEx(LOGFILTER, _T("curl_error=%i, curl_error_msg=%S, HttpCode=%i, UserReturnCode=%i\r\nURL=%S"),
			std::get<CurlErrorCode>(responseInfo_),
			std::get<CurlErrorMsg>(responseInfo_).c_str(),
			::get<HttpCode>(responseInfo_),
			::get<UserReturnCode>(responseInfo_),
			strUrl_.c_str());
		DumpSelfInfo(_T("OnFinsh:下载出错"), LOGL_Error);
	}
	else
	{
		DumpSelfInfo(_T("OnFinsh:下载完成"), LOGL_Error);
	}
	
	pDelegate_->OnFinish(bSuccess,taskID_, responseInfo_);
	//ioThreadPool_.postTask(std::bind(&IDelegate::OnFinish, pDelegate_,bSuccess, taskID_,info));
}

CDownloadTask::ResponseInfo CDownloadTask::GetResponseInfo(cpr::Response const & response)
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

bool CDownloadTask::DownLoadNextRange(int64_t const beg, int64_t const end)
{
	auto fileSize = end - beg;
	if (end != fileSize_)
	{
		if ((end & 0x7FFFF) != 0)
		{
			WYASSERT(false);
			LogErrorEx(LOGFILTER, _T("分块不对齐"));
		}
	}
	LogFinal(LOGFILTER, _T("Request next range:%llu"), beg);
	
	auto pHttpRequest = std::make_shared<CHttpRequest>(&hSession_);
	requestList_.insert(std::make_pair(beg, pHttpRequest));

	data::SaveDataPtr pData = std::make_shared<data::RecvData>();
	pData->first = beg;	
	pHttpRequest->setCookie(strCookie_);
	if (end != 0)
	{
		pHttpRequest->setRange(beg, end - 1);
	}
	else
	{
		pHttpRequest->setRange(beg, 0);
	}
	
	pHttpRequest->get(std::string(strUrl_), cpr::Parameters{}, CHttpRequest::RecvData_Body | CHttpRequest::RecvData_Header,
		std::bind(&CDownloadTask::OnRespond, this, std::placeholders::_1, std::placeholders::_2, pData,beg, fileSize),
		std::bind(&CDownloadTask::OnDataRecv, this, std::placeholders::_1, std::placeholders::_2, pData)
		);
	return true;
}

bool CDownloadTask::CreareFile()
{
	bool bRet = false;
	do
	{
		pSaveFile_ = WY::File::CreateAsioFile(ioThreadPool_.io_service(), strSavePath_.c_str(), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED);
		if (pSaveFile_.get() ==nullptr)
		{
			break;
		}
		auto hFile = pSaveFile_->native_handle();
		DWORD dwTemp = 0;
		DeviceIoControl(hFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwTemp, NULL);

		bRet = true;
	} while (false);
	return bRet;
}

void CDownloadTask::SetFile()
{
	auto hFile = pSaveFile_->native_handle();
	LARGE_INTEGER liOffset;
	liOffset.QuadPart = fileSize_;
	if (!SetFilePointerEx(hFile, liOffset, NULL, FILE_BEGIN))
	{
		return;
	}
	if (!SetEndOfFile(hFile))
	{
		return;
	}
}

#pragma region dump info

void CDownloadTask::DumpRespond(cpr::Response const & response)
{

}

void CDownloadTask::DumpSelfInfo(LPCTSTR strMsg,int logLevel)
{
	LogFinalEx(logLevel,LOGFILTER, _T("%s:taskID=%llu,filePath=%s\r\nURL=%S\r\nCookie=%S\r\nstrSHA=%S\r\nfileSize=%llu"),
		strMsg, taskID_, strSavePath_.c_str(), strUrl_.c_str(), strCookie_.c_str(), strSHA_.c_str(), fileSize_);
}

#pragma endregion dump info