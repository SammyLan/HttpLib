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
	std::string const & strCookie, std::string const & strSHA, uint64_t fileSize)
	:taskID_(taskID)
	,pDelegate_(pDelegate)
	,ioThreadPool_(ioThread)
	,nwThreadPool_(nwThread)
	,hSession_(hSession)
	,strUrl_(strUrl)
	,strCookie_(strCookie)
	,descFile_(strSavePath,fileSize,nThread, strSHA)
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

	if (descFile_.GetThreadCount() > 1)
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
			auto & info = descFile_.GetFileInfo();
			if (descFile_.GetFileSize() == 0)
			{
				info.fileSize = std::get<ContentLength>(ret);
			}
			else
			{
				if (descFile_.GetFileSize() != std::get<ContentLength>(ret))
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
	descFile_.CalcPiceInfo();
	recvSize = preRecvSize = descFile_.GetCompletedSize();

	DumpSelfInfo(_T("Start download"));
	if (!SetFile())
	{
		std::get<UserReturnCode>(responseInfo_) = ::GetLastError();
		DumpSelfInfo(_T("Set file failed"), LOGL_Error);
		OnFinish(false);
	}

	lastReportTime = nBeginDownload_ = ::GetTickCount();
	auto const & info = descFile_.GetFileInfo();
	auto const & piceInfo = info.pieceInfo;
	for (size_t i = 0; i < info.threadCount; ++i)
	{
		DownLoadNextRange(piceInfo[i]);
	}
}

void CDownloadTask::OnRespond(cpr::Response const & response, data::BufferPtr const & body, CDownloadTask::SaveDataPtr const& pData, uint64_t offset, uint64_t fileSize)
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
			if (fileSize == 0)//����Ҫ�ӷ�������ȡ�ļ���С
			{
				fileSize = descFile_.GetFileInfo().fileSize = recvSize;
			}
			WYASSERT(fileSize == recvSize);
			bSuccess = ((fileSize == recvSize) && (pData->first + pData->second.size() == offset + recvSize));
			if (!bSuccess)
			{
				WYASSERT(false);
				LogErrorEx(LOGFILTER, _T("[%llu]  ���ݳ��Ȳ���,�ļ�����Ϊ%llu,���ܳ���Ϊ%llu"),taskID_, pData->first + pData->second.size(), offset + recvSize);
				DumpSelfInfo(_T("�����С�����մ�С��һ��"), LOGL_Error);
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
		LogErrorEx(LOGFILTER, _T("[%llu]  ���س���:%S"),taskID_, headerStr.data());
		WYASSERT(false);
		Cancel();
		OnFinish(false);
	}
}

void CDownloadTask::OnDataRecv(data::byte const * data, size_t size, CDownloadTask::SaveDataPtr const& pData)
{
	recvSize += size;
	auto & pBuff = pData->second;
	auto curSize = pBuff.size();
	auto newSize = curSize + size;
	if (newSize >= s_save_size)
	{
		size_t appSize = size - (newSize%s_save_size);
		pBuff.append(data, appSize);

		SaveDataPtr  pTmp = std::make_shared<RecvData>();
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
			pDelegate_->OnProgress(taskID_, descFile_.GetFileSize(), recvSize, (size_t)speed);
		}
	}
}

void CDownloadTask::SaveData(CDownloadTask::SaveDataPtr const & pData, bool bDel)
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
	CDownloadTask::SaveDataPtr const & pData,bool bDel,
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
		DumpSelfInfo(_T("OnFinsh: Cancel���"), LOGL_Error);
		bSuccess = true;
	}
	if (!bSuccess)
	{
		LogErrorEx(LOGFILTER, _T("[%llu]  curl_error=%i, curl_error_msg=%S, HttpCode=%i, UserReturnCode=%i\r\nURL=%S"),taskID_,
			std::get<CurlErrorCode>(responseInfo_),
			std::get<CurlErrorMsg>(responseInfo_).c_str(),
			std::get<HttpCode>(responseInfo_),
			std::get<UserReturnCode>(responseInfo_),
			strUrl_.c_str());
		DumpSelfInfo(_T("OnFinsh:���س���"), LOGL_Error);
	}
	else
	{
		DumpSelfInfo(_T("OnFinsh:�������"), LOGL_Error);
		auto end = ::GetTickCount();
		auto time = end - nBeginDownload_;
		if (time == 0)
		{
			time = 1;
		}
		std::get<DownloadSpeed>(responseInfo_) = (size_t)(descFile_.GetFileSize() * 1000 / time);
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

bool CDownloadTask::DownLoadNextRange(CDownloadInfo::PieceInfo const & info)
{
	auto beg = info.offset + info.complectSize;
	auto end = info.offset + info.rangeSize;
	if ((info.offset & ~s_srv_block_size_and) != 0)
	{
		WYASSERT(false);
		LogErrorEx(LOGFILTER, _T("[%llu]  �ֿ鲻����"), taskID_);
	}
	

	LogFinal(LOGFILTER, _T("[%llu]  Request next range:%llu"), taskID_, beg);
	
	auto pHttpRequest = std::make_shared<CHttpRequest>(&hSession_);
	requestList_.insert(std::make_pair(info.offset, pHttpRequest));

	SaveDataPtr pData = std::make_shared<RecvData>();
	pData->first = beg;	
	pData->second.reserve(s_save_size);
	pHttpRequest->setCookie(strCookie_);
	pHttpRequest->setRange(beg, end - 1);
	
	
	pHttpRequest->get(std::string(strUrl_), cpr::Parameters{}, CHttpRequest::RecvData_Body | CHttpRequest::RecvData_Header,
		std::bind(&CDownloadTask::OnRespond, this, std::placeholders::_1, std::placeholders::_2, pData,info.offset, end - beg),
		std::bind(&CDownloadTask::OnDataRecv, this, std::placeholders::_1, std::placeholders::_2, pData)
		);
	return true;
}

bool CDownloadTask::CreareFile()
{
	bool bRet = false;
	do
	{
		pSaveFile_ = WY::File::CreateAsioFile(ioThreadPool_.io_service(), descFile_.GetDataFileName().c_str(), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED);
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
	liOffset.QuadPart = descFile_.GetFileSize();
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
		taskID_, strMsg, descFile_.GetFileName().c_str(), strUrl_.c_str(), strCookie_.c_str(), descFile_.GetDataFileName().c_str(), descFile_.GetFileInfo().fileSize);
}

#pragma endregion dump info