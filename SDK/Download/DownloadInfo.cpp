#include "stdafx.h"
#include "DownloadInfo.h"
#include <boost/filesystem.hpp> 
#include <Include/WYUtil.h>
#include <winioctl.h>

#ifdef min
#undef min
#endif // min

uint64_t const s_block_size = 1024 * 1024 * 1; //10M
uint64_t const s_srv_block_size = 1024 * 512;
uint64_t const s_srv_block_size_and = ~(s_srv_block_size - 1);

#define PIECE_DESC_SUFFIX		L".qmfpd2"
#define DOWNLOAD_DATA_SUFFIX	L".qmdt"

static TCHAR LOGFILTER[] = _T("CDownloadPieceDesc");

CDownloadInfo::CDownloadInfo(std::wstring const & filePath, uint64_t fileSize, size_t threadCount, std::string const & sha)
	:filePath_(filePath)
	,strSHA_(sha)
	,fileSize_(fileSize)
	,threadCount_(threadCount)
{
	if (threadCount_ == 0)
	{
		threadCount_ = 1;
	}
}

CDownloadInfo::~CDownloadInfo()
{
	if (fileRegion_.get() != nullptr)
	{
		Flush();
		LogFinal(LOGFILTER, _T("保存分片信息"));
	}
}

bool CDownloadInfo::InitDownload(boost::asio::io_service& io_service)
{
	if (fileSize_ == 0)
	{
		lastError_ = WYErrorCode::DOWNERR_FILESIZE_ZERO;
		WYASSERT(false);
		return false;
	}

	if (strSHA_.size() != SHASize)
	{
		lastError_ = WYErrorCode::DOWNERR_SHALEN;
		WYASSERT(false);
		return false;
	}

	if (IskNeedReset())
	{
		if (!TryToDelTmpFile())
		{
			return false;
		}

		FileInfo info;
		memset(&info, 0, sizeof(info));
		info.dwFlag = DownloadInfo::FileFlag;
		info.infoSize = sizeof(info);
		info.fileSize = fileSize_;
		memcpy(&info.sha, strSHA_.data(), SHASize);
		
		threadCount_ = (size_t)std::min((uint64_t)threadCount_, fileSize_ / s_block_size);
		if (threadCount_ == 0)
		{
			threadCount_ = 1;
		}

		info.threadCount = (uint16_t)threadCount_;
		uint64_t block_size = fileSize_ / threadCount_;

		uint64_t beg = 0;
		auto & pieceInfo = info.pieceInfo;
		for (size_t i = 0; i < threadCount_; i++)
		{
			beg = ((beg + s_srv_block_size - 1) & s_srv_block_size_and);
			pieceInfo[i].offset = beg;
			pieceInfo[i].complectSize = 0;
			beg += block_size;
		}
		pieceInfo[threadCount_ - 1].rangeSize = fileSize_ - pieceInfo[threadCount_ - 1].offset;
		for (size_t i = 0; i < threadCount_ - 1; i++)
		{
			pieceInfo[i].rangeSize = pieceInfo[i + 1].offset - pieceInfo[i].offset;
		}

#ifdef _DEBUG
		{
			uint64_t totalSize = 0;
			for (size_t i = 0; i < threadCount_; i++)
			{
				totalSize += pieceInfo[i].rangeSize;
			}
			WYASSERT(totalSize == fileSize_);
		}		
#endif _DEBUG
		if (!SaveToFile(info))
		{
			return false;
		}
	}

	if (!OpenDataFile(io_service) ||!OpenDescFile())
	{
		return false;
	}
	
	return true;
	
}

bool CDownloadInfo::OnDownloadFin()
{
	pInfo_ = nullptr;
	fileRegion_.reset();
	fileMaping_.reset();
	pSaveFile_.reset();
	bool bRet = true;
	try
	{
		boost::filesystem::path desc(GetDescFileName());
		boost::filesystem::path data(GetDataFileName());
		boost::filesystem::path file(filePath_);
		boost::filesystem::remove(desc);
		boost::filesystem::rename(data, file);
	}
	catch (const std::exception& e)
	{
		CWYString error = CA2W(e.what());
		LogErrorEx(LOGFILTER, _T("error:%s"), (LPCTSTR)error);
		lastError_ = ::GetLastError();
		bRet = false;
	}
	return bRet;
}

bool CDownloadInfo::SaveToFile(FileInfo const & info)
{
	boost::filesystem::path desc(GetDescFileName());
	ofstream file(desc.c_str(), ios_base::out | ios_base::binary);
	if (!file)
	{
		lastError_ = ::GetLastError();
		WYASSERT(false);
		return false;	
	}

	file.seekp(0, ios_base::beg);
	file.write((char const*)&info, sizeof(info));
	return true;
}

bool CDownloadInfo::IskNeedReset()
{
	boost::filesystem::path desc(GetDescFileName());
	boost::filesystem::path data(GetDataFileName());

	bool needReset = true;
	if (boost::filesystem::exists(desc) && boost::filesystem::exists(desc))
	{
		FileInfo info;
		memset(&info, 0, sizeof(info));
		fstream file(desc.c_str(), ios_base::in| ios_base::out | ios_base::binary);
		if (!file)
		{
			return true;
		}
		file.read((char*)& info, sizeof(info));
		if (info.threadCount < MaxThreadCount
			&& (info.infoSize == sizeof(FileInfo))
			&& (info.dwFlag == DownloadInfo::FileFlag)
			&& (info.fileSize == fileSize_)
			&& (memcmp(&info.sha, strSHA_.data(), SHASize) == 0))
		{
			needReset = false;
			if (threadCount_ != info.threadCount)
			{
				if (info.fileSize == 1 && threadCount_ > 1)
				{
					//:TODO asjust
				}
				threadCount_ = info.threadCount;
				file.seekp(0, ios_base::beg);
				file.write((char const*)&info, sizeof(info));
			}
		}
	}
	return needReset;
}

bool CDownloadInfo::TryToDelTmpFile()
{
	bool bRet = true;
	try
	{
		boost::filesystem::path desc(GetDataFileName());
		boost::filesystem::path data(GetDescFileName());
		if (boost::filesystem::exists(desc))
		{
			boost::filesystem::remove(desc);
		}
		if (boost::filesystem::exists(data))
		{
			boost::filesystem::remove(data);
		}
	}
	catch (const std::exception& e)
	{
		CWYString error = CA2W(e.what());
		LogErrorEx(LOGFILTER, _T("error:%s"), (LPCTSTR)error);
		lastError_ = ::GetLastError();
		bRet = false;
	}
	return bRet;
}

uint64_t CDownloadInfo::GetCompletedSize() const
{
	uint64_t size = 0;
	for (size_t i = 0; i < pInfo_->threadCount; i++)
	{
		size += pInfo_->pieceInfo[i].complectSize;
	}
	return size;
}

std::wstring  CDownloadInfo::GetDataFileName() const
{
	return filePath_ + DOWNLOAD_DATA_SUFFIX;
}

std::wstring  CDownloadInfo::GetDescFileName() const
{
	return filePath_ + PIECE_DESC_SUFFIX;
}

std::wstring const & CDownloadInfo::GetFileName() const
{
	return filePath_;
}

void CDownloadInfo::Flush(bool bForce)
{
	if (fileRegion_.get() != nullptr)
	{
		uint64_t cur = ::GetTickCount();
		bool bFlush = bForce || (cur - lastFlush_) > 1000 * 2;
		if (bFlush)
		{
			fileRegion_->flush();
			lastFlush_ = cur;
		}	
	}	
}

bool CDownloadInfo::OpenDataFile(boost::asio::io_service& io_service)
{
	bool bRet = false;
	do
	{
		
		auto && rt= WY::File::CreateAsioFile(io_service, GetDataFileName().c_str(), GENERIC_WRITE, FILE_SHARE_READ, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED);
		pSaveFile_ = rt.first;		
		if (pSaveFile_.get() == nullptr)
		{
			break;
		}
		int iLastError = rt.second;
		if (iLastError == ERROR_SUCCESS)
		{
			auto hFile = pSaveFile_->native_handle();
			DWORD dwTemp = 0;
			if (!DeviceIoControl(hFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwTemp, NULL))
			{
				break;
			}

			LARGE_INTEGER liOffset;
			liOffset.QuadPart = GetFileSize();
			if (!SetFilePointerEx(hFile, liOffset, NULL, FILE_BEGIN))
			{
				break;
			}
			if (!SetEndOfFile(hFile))
			{
				break;
			}
		}		
		bRet = true;
	} while (false);
	if (!bRet)
	{
		lastError_ = ::GetLastError();
		WYASSERT(false);
	}
	return bRet;
}

bool CDownloadInfo::OpenDescFile()
{
	bool bRet = true;
	try
	{
		std::string strFile = CW2A(GetDescFileName().c_str());
		fileMaping_ = std::make_shared<file_mapping>(strFile.c_str(), boost::interprocess::read_write);
		fileRegion_ = std::make_shared<mapped_region>(*fileMaping_.get(), boost::interprocess::read_write);

		pInfo_ = (FileInfo*)fileRegion_->get_address();
		std::size_t size = fileRegion_->get_size();
		if (size != DownloadInfo::FileInfoSize)
		{
			bRet = false;
		}
	}
	catch (const std::exception& e)
	{
		CWYString error = CA2W(e.what());
		LogErrorEx(LOGFILTER, _T("error:%s"), (LPCTSTR)error);
		lastError_ = ::GetLastError();
		bRet = false;		
	}
	WYASSERT(bRet);
	return bRet;
}