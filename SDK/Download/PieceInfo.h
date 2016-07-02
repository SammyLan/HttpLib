#pragma once

namespace DownloadInfo
{
	enum 
	{ 
		SHASize = 40,
		PieceInfoSize = 32,
		FileInfoSize = 384,
		MaxThreadCount = 10
	};

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
};

static_assert(sizeof(DownloadInfo::PieceInfo) == DownloadInfo::PieceInfoSize, "size of PieceInfo must be 32");
static_assert(sizeof(DownloadInfo::FileInfo) == DownloadInfo::FileInfoSize, "size of FileInfo must be 384");