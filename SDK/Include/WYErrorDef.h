#pragma once

namespace WYErrorCode
{
	enum WYError
	{
		CURLERR_BEGIN = 100000, //CURL错误
		HTTPERR_BEGIN = 101000, //HTTP错误

		DOWNERR_BEGIN = 1020000,
		DOWNERR_SIZE_CONFLICT = DOWNERR_BEGIN + 1,	//文件大小不一致
		DOWNERR_RECV_SIZE_CONFLICT = DOWNERR_BEGIN + 2,	//接收数据大小不一致
		DOWNERR_FILESIZE_ZERO	= DOWNERR_BEGIN + 3,	//空文件下载
		DOWNERR_OPENFILEMAPING	= DOWNERR_BEGIN + 4,	//打开内存映射文件失败
		DOWNERR_SHALEN			= DOWNERR_BEGIN + 5,	//SHA大小不对



		UPERROR_BEGIN = 103000,
	};
}