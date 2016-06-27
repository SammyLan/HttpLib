#include "stdafx.h"
#include "DownloadTask.h"
#include <sstream>
#include <http/HttpGlobal.h>
#include <winioctl.h>
#include "WYErrorDef.h"
#ifdef min
#undef min
#endif // min

static TCHAR LOGFILTER[] = _T("CDownloadTask");

size_t const s_save_size = 1024 * 128;
size_t const s_block_size = 1024 * 1024 * 1; //10M
size_t const s_srv_block_size = 1024 * 512;
size_t const s_srv_block_size_and = ~(s_srv_block_size - 1);

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
		DumpSelfInfo(_T("Create file failed"), LOGL_Error);
		OnFinish(false);
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
	pDelegate_ = nullptr;
	Cancel();
	DumpSelfInfo(_T("Cancel task"));
}

void CDownloadTask::Cancel()
{
	for (auto it = requestList_.begin(); it != requestList_.end(); it++)
	{
		it->second->cancel();
	}
}

void CDownloadTask::GetFileInfo()
{
	auto pHttpRequest = std::make_shared<CHttpRequest>(&hSession_);
	requestList_.insert(std::make_pair(0, pHttpRequest));
	if (!strCookie_.empty())
	{
		pHttpRequest->setCookie(strCookie_);
	}

	pHttpRequest->headRequest(std::string(strUrl_), cpr::Parameters{},
		[beg = WYTime::g_watch.Now(),this](cpr::Response const & response, data::BufferPtr const & body)
	{
		LockGuard gurad(csLock_);
		
		int64_t end = WYTime::g_watch.Now();
		LogFinal(LOGFILTER, _T("[%llu]  take time=%llu"),taskID_, end - beg);

		auto pRequest = *requestList_.begin();
		requestList_.clear();
		auto && ret = GetResponseInfo(response);
		if (bCancel_)
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
	if (!SetFile())
	{
		std::get<UserReturnCode>(responseInfo_) = ::GetLastError();
		DumpSelfInfo(_T("Set file failed"), LOGL_Error);
		OnFinish(false);
	}

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
		beg = ((beg + s_srv_block_size - 1) & s_srv_block_size_and);
		blockInfo.push_back(beg);
		beg += block_size;
	}
	if (beg >= fileSize_)
	{
		beg = fileSize_;
		blockInfo.push_back(beg);
	}
	WYASSERT((nThread + 1) == blockInfo.size());
	lastReportTime = ::GetTickCount();
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
				fileSize = fileSize_ = recvSize;
			}
			WYASSERT(fileSize == recvSize);
			bSuccess = ((fileSize == recvSize) && (pData->first + pData->second.size() == offset + recvSize));
			if (!bSuccess)
			{
				WYASSERT(false);
				LogErrorEx(LOGFILTER, _T("[%llu]  数据长度不对,文件长度为%llu,接受长度为%llu"),taskID_, pData->first + pData->second.size(), offset + recvSize);
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
		LogErrorEx(LOGFILTER, _T("[%llu]  下载出错:%S"),taskID_, headerStr.data());
		WYASSERT(false);
		Cancel();
		OnFinish(false);
	}
}

void CDownloadTask::OnDataRecv(data::byte const * data, size_t size, data::SaveDataPtr const& pData)
{
	recvSize += size;
	auto & pBuff = pData->second;
	auto curSize = pBuff.size();
	auto newSize = curSize + size;
	if (newSize >= s_save_size)
	{
		size_t appSize = size - (newSize%s_save_size);
		pBuff.append(data, appSize);

		data::SaveDataPtr  pTmp = std::make_shared<data::RecvData>();
		pTmp->first = pData->first;
		pTmp->second.reserve(s_save_size);
		pData->first += pBuff.size();
		pTmp->second.swap(pBuff);
		pBuff.assign(data + appSize, size - appSize);
		SaveData(pTmp);
	}
	else
	{
		pBuff.append(data, size);
	}
	int64_t cur = ::GetTickCount();
	if ((cur - lastReportTime) > reportInterval)
	{
		auto speed = (recvSize - preRecvSize)* 1000/(cur - lastReportTime);
		lastReportTime = cur;
		preRecvSize = recvSize;
		auto pDelegate = pDelegate_;
		if (pDelegate)
		{
			pDelegate_->OnProgress(taskID_, fileSize_, recvSize, (size_t)speed);
		}
	}
}

void CDownloadTask::SaveData(data::SaveDataPtr const & pData, bool bDel)
{
	
	auto beg = WYTime::g_watch.NowInMicro();
	ioThreadPool_.postTask(
		[pData,bDel, pThis = shared_from_this()]()
	{
		auto & pBuff = pData->second;
		auto size = pBuff.size();
		pThis->pSaveFile_->async_write_some_at(pData->first, boost::asio::buffer(pBuff.data(), size),
			std::bind(&CDownloadTask::OnDataSaveHandler, pThis, pData, bDel, std::placeholders::_1, std::placeholders::_2, size));
	}
	);
}

void CDownloadTask::OnDataSaveHandler(
	data::SaveDataPtr const & pData,bool bDel,
	const boost::system::error_code& error, // Result of operation.
	std::size_t bytes_transferred,           // Number of bytes written.
	std::size_t nBlockIndex
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
		LogErrorEx(LOGFILTER, _T("[%llu]  curl_error=%i, curl_error_msg=%S, HttpCode=%i, UserReturnCode=%i\r\nURL=%S"),taskID_,
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
	if (pDelegate_ != nullptr)
	{
		pDelegate_->OnFinish(taskID_,bSuccess, responseInfo_);
	}
	
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
			LogErrorEx(LOGFILTER, _T("[%llu]  can't find the value of Content-Length"),taskID_);
		}

		auto itRet = header.find("User-ReturnCode");
		if (itRet != header.end())
		{
			std::get<UserReturnCode>(res) = atoi(itRet->second.c_str());
		}
		else
		{
			LogErrorEx(LOGFILTER, _T("[%llu]  can't find the value of User-ReturnCode"),taskID_);
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
		if ((end & ~s_srv_block_size_and) != 0)
		{
			WYASSERT(false);
			LogErrorEx(LOGFILTER, _T("[%llu]  分块不对齐"),taskID_);
		}
	}
	LogFinal(LOGFILTER, _T("[%llu]  Request next range:%llu"), taskID_,beg);
	
	auto pHttpRequest = std::make_shared<CHttpRequest>(&hSession_);
	requestList_.insert(std::make_pair(beg, pHttpRequest));

	data::SaveDataPtr pData = std::make_shared<data::RecvData>();
	pData->first = beg;	
	pData->second.reserve(s_save_size);
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
		if (!DeviceIoControl(hFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwTemp, NULL))
		{
			break;
		}

		bRet = true;
	} while (false);
	return bRet;
}

bool CDownloadTask::SetFile()
{
	auto hFile = pSaveFile_->native_handle();
	LARGE_INTEGER liOffset;
	liOffset.QuadPart = fileSize_;
	if (!SetFilePointerEx(hFile, liOffset, NULL, FILE_BEGIN))
	{
		return false;
	}
	if (!SetEndOfFile(hFile))
	{
		return false;
	}
	return true;
}

#pragma region dump info

void CDownloadTask::DumpRespond(cpr::Response const & response)
{

}

void CDownloadTask::DumpSelfInfo(LPCTSTR strMsg,int logLevel)
{
	LogFinalEx(logLevel,LOGFILTER, _T("[%llu]  %s:filePath=%s\r\nURL=%S\r\nCookie=%S\r\nstrSHA=%S\r\nfileSize=%llu"),
		taskID_, strMsg, strSavePath_.c_str(), strUrl_.c_str(), strCookie_.c_str(), strSHA_.c_str(), fileSize_);
}

#pragma endregion dump info