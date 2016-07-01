#pragma once
#include <memory>
#include <string>
#include <File/WYFile.h>

class CDownloadInfo
{
public:
	enum 
	{ 
		SHASize = 40,
		PieceInfoSize = 32,
		FileInfoSize = 384,
		MaxThreadCount = 10
	};
public:
	struct PieceInfo
	{
		uint64_t offset;
		uint64_t rangeSize;
		uint64_t complectSize;
		byte reserve[sizeof(uint64_t)];
	};

	struct FileInfo
	{
		uint32_t dwFlag;		//1258169257
		uint16_t infoSize;		//结构体大小
		uint16_t threadCount;	//线程数
		uint64_t fileSize;		//文件ID		
		char	 sha[SHASize];		//SHA
		byte reserve[sizeof(uint64_t)];
		PieceInfo pieceInfo[MaxThreadCount];
	};
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