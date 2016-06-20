#include "StdAfx.h"
#include "AsyncFile.h"
#include <cassert>

typedef BOOL(WINAPI * TCancelIoEx)(HANDLE, LPOVERLAPPED);
TCancelIoEx  GetCancelIoExProc()
{
	HMODULE hModule = ::GetModuleHandle(_T("Kernel32.dll"));
	if (hModule == NULL)
	{
		return NULL;
	}
	TCancelIoEx CancelIoEx = (TCancelIoEx)GetProcAddress(hModule, ("CancelIoEx"));
	return CancelIoEx;
}

TCancelIoEx S_CancelIoEx = GetCancelIoExProc();
CAsyncFile::CAsyncFile()
{

}

CAsyncFile::~CAsyncFile()
{
	CancelIO();
	fileHandle_.reset();
	m_h = nullptr;
}

BOOL CAsyncFile::Create(
	boost::asio::io_service& io_service,
	_In_z_ LPCTSTR szFilename,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ LPSECURITY_ATTRIBUTES lpsa,
	_In_opt_ HANDLE hTemplateFile) throw()
{
	auto hr = base::Create(szFilename, dwDesiredAccess
		, dwShareMode,dwCreationDisposition,dwFlagsAndAttributes| FILE_FLAG_OVERLAPPED
		, lpsa, hTemplateFile);
	BOOL bRet = (hr == S_OK);
	if (bRet)
	{
		fileHandle_ = std::make_shared<asio_file>(io_service, m_h);
	}
	return bRet;
}

int CAsyncFile::Read(
	_Out_writes_bytes_(nBufSize) LPVOID pBuffer,
	_In_ DWORD nBufSize,
	_Inout_opt_ LPOVERLAPPED pOverlapped) throw()
{
	auto hr = base::Read(pBuffer, nBufSize, pOverlapped);
	int iError = GetLastError();
	if (hr == S_OK || iError == ERROR_IO_PENDING)
	{
		iError = 0;
	}
	else
	{
		assert(0);
	}
	return iError;
}

int CAsyncFile::Write(
	_In_reads_bytes_(nBufSize) LPCVOID pBuffer,
	_In_ DWORD nBufSize,
	_Inout_opt_ LPOVERLAPPED pOverlapped) throw()
{

	auto hr =  base::Write(pBuffer, nBufSize, pOverlapped);
	int iError = GetLastError();
	if (hr == S_OK || iError == ERROR_IO_PENDING)
	{
		iError = 0;
	}
	else
	{
		assert(0);
	}
	return iError;
}

ULONGLONG CAsyncFile::GetSize() const throw()
{
	ULONGLONG uSize = 0;
	auto hr = base::GetSize(uSize);
	if (hr != S_OK)
	{
		uSize = 0;
	}

	return uSize;
}


BOOL CAsyncFile::CancelIO()
{
	if (S_CancelIoEx)
	{
		S_CancelIoEx(m_h, NULL);
	}
	else
	{
		CancelIo(m_h);
	}
	return TRUE;
}


WY::File::AsioFilePtr WY::File::CreateAsioFile(
	boost::asio::io_service& io_service,
	_In_z_ LPCTSTR szFilename,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode ,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ LPSECURITY_ATTRIBUTES lpsa ,
	_In_opt_ HANDLE hTemplateFile )
{
	CAtlFile file;
	WY::File::AsioFilePtr pFile;
	auto hr = file.Create(szFilename, dwDesiredAccess
		, dwShareMode, dwCreationDisposition, dwFlagsAndAttributes | FILE_FLAG_OVERLAPPED
		, lpsa, hTemplateFile);
	BOOL bRet = (hr == S_OK);
	if (bRet)
	{
		pFile = std::make_shared<AsioFile>(io_service, file.Detach());
	}
	return pFile;
}