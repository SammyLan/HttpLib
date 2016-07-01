#pragma once
#include <atlfile.h>
#include <boost\asio.hpp>
#include <boost/interprocess/file_mapping.hpp>  
#include <boost/interprocess/mapped_region.hpp>  
#include <memory>

namespace WY
{
	namespace File
	{
		typedef boost::asio::windows::random_access_handle AsioFile;
		typedef std::shared_ptr<AsioFile> AsioFilePtr;

		AsioFilePtr  CreateAsioFile(
			boost::asio::io_service& io_service,
			_In_z_ LPCTSTR szFilename,
			_In_ DWORD dwDesiredAccess = GENERIC_READ,
			_In_ DWORD dwShareMode = FILE_SHARE_READ,
			_In_ DWORD dwCreationDisposition = OPEN_EXISTING,
			_In_ DWORD dwFlagsAndAttributes = FILE_FLAG_OVERLAPPED,
			_In_opt_ LPSECURITY_ATTRIBUTES lpsa = NULL,
			_In_opt_ HANDLE hTemplateFile = NULL);

		BOOL CancelIo(_In_ HANDLE hFile,	_In_opt_ LPOVERLAPPED lpOverlapped);

		BOOL GetFileSize(_In_  HANDLE hFile, _Out_ uint64_t & fileSize);
	}
}

