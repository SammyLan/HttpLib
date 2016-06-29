#include "StdAfx.h"
#include "WYFile.h"
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


BOOL WY::File::CancelIo(_In_ HANDLE hFile, _In_opt_ LPOVERLAPPED lpOverlapped)
{
	if (S_CancelIoEx != NULL)
	{
		return ::S_CancelIoEx(hFile, lpOverlapped);
	}
	return ::CancelIo(hFile);
}

BOOL WY::File::GetFileSize(_In_  HANDLE hFile, _Out_ uint64_t & fileSize)
{
	LARGE_INTEGER li;
	BOOL bRet = ::GetFileSizeEx(hFile, &li);
	if (bRet)
	{
		fileSize = li.QuadPart;
	}
	else
	{
		fileSize = 0;
	}
	return bRet;
}