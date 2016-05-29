#pragma once
#include "IWYUploadSDK.h"
#include <string>
using namespace std;

struct CInitInfo:public IInitInfo
{
	MEMFUNC_IMPL(UINT32, connCount);
	MEMFUNC_IMPL(size_t, MaxThread);
	MEMFUNC_IMPL_STR(LocalPath);
	MEMFUNC_IMPL_STR(OsType);
	MEMFUNC_IMPL_STR(OsVersion);
	MEMFUNC_IMPL_STR(AppVersion);
	MEMFUNC_IMPL_STR(QUA);
	MEMFUNC_IMPL_STR(ServerUrl);
};

struct CUploadInfo: public IUploadInfo
{
	//MEMFUNC_IMPL(WYTASKID, TaskID);
	MEMFUNC_IMPL(ll, Uin);
	MEMFUNC_IMPL_STR(FileID);
	MEMFUNC_IMPL_STR(CheckKey);
	MEMFUNC_IMPL(PFileSignInfo, FileSignInfo);
	MEMFUNC_IMPL_STR(ServerName);
	MEMFUNC_IMPL_STR(ServerIP);
	MEMFUNC_IMPL(int, ServerPort);
	MEMFUNC_IMPL(int, ChannelCount);
	MEMFUNC_IMPL_STRW(FilePath);
	MEMFUNC_IMPL(PIUploadDelegate, Delegate);
};