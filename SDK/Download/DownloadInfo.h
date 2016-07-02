#pragma once
#include <memory>
#include <string>
#include <File/WYFile.h>
#include "PieceInfo.h"
#include <Include/WYErrorDef.h>
#include <boost/interprocess/file_mapping.hpp>  
#include <boost/interprocess/mapped_region.hpp>
using namespace DownloadInfo;

class CDownloadInfo
{
	typedef boost::interprocess::file_mapping file_mapping;
	typedef boost::interprocess::mapped_region mapped_region;
	typedef std::shared_ptr<file_mapping>  file_mappingPtr;
	typedef std::shared_ptr<mapped_region> mapped_regionPtr;
public:
	CDownloadInfo(std::wstring const & filePath, uint64_t fileSize, size_t threadCount, std::string const & sha);
	~CDownloadInfo();

	bool InitDownload(boost::asio::io_service& io_service);
	std::wstring  const & GetFileName() const;
	std::string  const & GeSHA() const { return strSHA_; }
	uint64_t GetFileSize() const { return fileSize_; }
	uint64_t GetCompletedSize() const;
	size_t GetThreadCount() const { return threadCount_; }
	void Flush(bool bForce = false);
	FileInfo * GeDownloadInfo() { return pInfo_; }
	WY::File::AsioFilePtr & GetFile() { return pSaveFile_; }
	int GetLastError() const { return lastError_; }
	bool OnDownloadFin();
private:
	bool IskNeedReset();
	bool TryToDelTmpFile();
	std::wstring  GetDataFileName() const;
	std::wstring  GetDescFileName() const;
	bool OpenDataFile(boost::asio::io_service& io_service);
	bool OpenDescFile();
	bool SaveToFile(FileInfo const & info);
private:	
	std::wstring const filePath_;
	std::string const strSHA_;
	uint64_t	fileSize_;
	size_t		threadCount_;
	int			lastError_ = 0;
	WY::File::AsioFilePtr pSaveFile_;
	file_mappingPtr		fileMaping_;
	mapped_regionPtr	fileRegion_;
	FileInfo*			pInfo_ = nullptr;
	uint64_t			lastFlush_ = 0;
};