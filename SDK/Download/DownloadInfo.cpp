#include "stdafx.h"
#include "DownloadInfo.h"
#include <boost/filesystem.hpp> 
#include <boost/interprocess/file_mapping.hpp>  
#include <boost/interprocess/mapped_region.hpp> 
#include <Include/WYUtil.h>

#ifdef min
#undef min
#endif // min

static_assert(sizeof(CDownloadInfo::PieceInfo) == CDownloadInfo::PieceInfoSize,"size of PieceInfo must be 32");
static_assert(sizeof(CDownloadInfo::FileInfo) == CDownloadInfo::FileInfoSize, "size of FileInfo must be 384");
static const uint32_t s_dwFlag = 1258169257;

uint64_t const s_block_size = 1024 * 1024 * 1; //10M
uint64_t const s_srv_block_size = 1024 * 512;
uint64_t const s_srv_block_size_and = ~(s_srv_block_size - 1);

#define PIECE_DESC_SUFFIX		L".qmfpd2"
#define DOWNLOAD_DATA_SUFFIX	L".qmdt"

static TCHAR LOGFILTER[] = _T("CDownloadPieceDesc");

CDownloadInfo::CDownloadInfo(std::wstring const & filePath, uint64_t fileSize, size_t threadCount, std::string const & sha)
	:filePath_(filePath)
	,strSHA_(sha)
{
	memset(&info_, 0, sizeof(info_));
	info_.dwFlag = s_dwFlag;
	info_.infoSize = sizeof(FileInfo);
	info_.fileSize = fileSize;
	
	if (sha.size() == SHASize)
	{
		memcpy(&info_.sha, sha.data(), SHASize);
	}
	else
	{
		WYASSERT(false);
	}
	if (threadCount == 0)
	{
		threadCount = 1;
	}
	info_.threadCount = (uint16_t)threadCount;
}

bool CDownloadInfo::CalcPiceInfo()
{
	boost::filesystem::path desc( GetDescFileName());
	boost::filesystem::path data( GetDataFileName() );

	bool needReset = true;
	if (boost::filesystem::exists(desc) && boost::filesystem::exists(desc))
	{
		FileInfo tmpInfo;
		memset(&tmpInfo, 0, sizeof(tmpInfo));
		ifstream file(desc.c_str(), ios_base::in|ios_base::binary);
		file.read((char*)& tmpInfo,sizeof(tmpInfo));

		if (tmpInfo.threadCount < MaxThreadCount
			&& (tmpInfo.infoSize == sizeof(FileInfo))
			&& (tmpInfo.dwFlag == s_dwFlag)
			&& (tmpInfo.fileSize != info_.fileSize)
			&& (memcmp(&tmpInfo.sha, info_.sha, SHASize) != 0))
		{
			needReset = false;
		}
		else
		{
			info_ = tmpInfo;
		}
	}
	if (needReset)
	{
		auto const  fileSize = info_.fileSize;
		size_t nThread = (size_t)std::min((uint64_t)info_.threadCount, fileSize / s_block_size);
		if (nThread == 0)
		{
			nThread = 1;
		}

		info_.threadCount = (uint16_t)nThread;		
		uint64_t block_size = fileSize / nThread;

		uint64_t beg = 0;
		auto & pieceInfo = info_.pieceInfo;
		for (size_t i = 0; i < nThread; i++)
		{
			beg = ((beg + s_srv_block_size - 1) & s_srv_block_size_and);
			pieceInfo[i].offset = beg;
			pieceInfo[i].complectSize = 0;
			beg += block_size;
		}
		pieceInfo[nThread - 1].rangeSize = fileSize - pieceInfo[nThread - 1].offset;
		for (size_t i = 0; i < nThread - 1; i++)
		{
			pieceInfo[i].rangeSize = pieceInfo[i + 1].offset - pieceInfo[i].offset;
		}

#ifdef _DEBUG
		{
			uint64_t totalSize = 0;
			for (size_t i = 0; i < nThread; i++)
			{
				totalSize += pieceInfo[i].rangeSize;
			}
			WYASSERT(totalSize == fileSize);
		}
		
#endif _DEBUG
		Save();
	}

	try
	{
		std::string strFile = CW2A(GetDescFileName().c_str());
		boost::interprocess::file_mapping m_file(strFile.c_str(), boost::interprocess::read_write);
		boost::interprocess::mapped_region region(m_file, boost::interprocess::read_write);

		void * addr = region.get_address();
		std::size_t size = region.get_size();
	}
	catch (const std::exception& e)
	{
		CWYString error = CA2W(e.what());
		LogErrorEx(LOGFILTER, _T("error:%s"), (LPCTSTR)error);
	}

	return true;
	
}
void CDownloadInfo::TryToDelTmpFile()
{
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
	catch (const std::exception&)
	{

	}
	
}

uint64_t CDownloadInfo::GetCompletedSize() const
{
	uint64_t size = 0;
	for (size_t i = 0; i < info_.threadCount; i++)
	{
		size += info_.pieceInfo[i].complectSize;
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

void CDownloadInfo::Save()
{
	boost::filesystem::path desc(GetDescFileName());
	ofstream file(desc.c_str(), ios_base::out | ios_base::binary);
	if (file)
	{
		file.seekp(0, ios_base::beg);
		file.write((char *)&info_, sizeof(info_));
	}
}