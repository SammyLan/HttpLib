#pragma once
#include <memory>
#include <string>
#include <File/WYFile.h>
#include "PieceInfo.h"
using namespace DownloadInfo;

class CDownloadInfo
{
public:
	CDownloadInfo(std::wstring const & filePath, uint64_t fileSize, size_t threadCount, std::string const & sha);
	~CDownloadInfo();

	bool InitDownload(boost::asio::io_service& io_service);
	std::wstring  GetDataFileName() const;
	std::wstring  const & GetFileName() const;
	std::string  const & GeSHA() const { return strSHA_; }
	uint64_t GetFileSize() const { return info_.fileSize; }
	uint64_t GetCompletedSize() const;
	uint16_t GetThreadCount() const { return info_.threadCount; }
	void Save();
	FileInfo * GetFileInfo() { return &info_; }
	WY::File::AsioFilePtr & GetFile() { return pSaveFile_; }
	int GetLastError() const { return lastError_; }
private:
	void TryToDelTmpFile();
	std::wstring  GetDescFileName() const;
	bool CreareFile(boost::asio::io_service& io_service);
private:
	FileInfo info_;
	WY::File::AsioFilePtr pSaveFile_;
	std::wstring const filePath_;
	std::string const strSHA_;
	int lastError_ = 0;
};