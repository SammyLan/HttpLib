#pragma once
#include <atlfile.h>
#include <boost\asio.hpp>
#include <memory>

class CAsyncFile:protected CAtlFile
{
public:
	typedef boost::asio::windows::random_access_handle asio_file;
	typedef CAtlFile base;
	using base::GetOverlappedResult;
	using base::Seek;
	using base::GetPosition;
	using base::Flush;
	using base::LockRange;
	using base::UnlockRange;
	using base::SetSize;
	using base::Close;
public:
	 CAsyncFile();
	~CAsyncFile();
	BOOL Create(
		boost::asio::io_service& io_service,
		_In_z_ LPCTSTR szFilename,
		_In_ DWORD dwDesiredAccess = GENERIC_READ,
		_In_ DWORD dwShareMode = FILE_SHARE_READ,
		_In_ DWORD dwCreationDisposition = OPEN_EXISTING,
		_In_ DWORD dwFlagsAndAttributes = FILE_FLAG_OVERLAPPED,
		_In_opt_ LPSECURITY_ATTRIBUTES lpsa = NULL,
		_In_opt_ HANDLE hTemplateFile = NULL) throw();

	int Read(
		_Out_writes_bytes_(nBufSize) LPVOID pBuffer,
		_In_ DWORD nBufSize,
		_Inout_opt_ LPOVERLAPPED pOverlapped) throw();

	int Write(
		_In_reads_bytes_(nBufSize) LPCVOID pBuffer,
		_In_ DWORD nBufSize,
		_Inout_opt_ LPOVERLAPPED pOverlapped) throw();

	ULONGLONG GetSize() const throw();
	BOOL CancelIO();
	CAsyncFile::asio_file & getAsioFile() { return *fileHandle_; }
private:	
	std::shared_ptr<asio_file> fileHandle_;
};

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
	}
}

