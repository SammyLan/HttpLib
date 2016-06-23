#pragma once

namespace WYErrorCode
{
	enum WYErrorError
	{
		WYErrorError_Begin = 800000,
		Download_HeadSizeConflict = 800001,	//文件大小不一致
		Download_RequestSizeConflict = 800002,	//文件大小不一致
	};
}