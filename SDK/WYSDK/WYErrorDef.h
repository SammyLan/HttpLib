#pragma once

namespace WYError
{
enum WYErrorCode
{
	WYErrorCode_Begin = 1000000,
	WYErrorCode_UnkError,			//未知错误
	WYErrorCode_NetError,			//网络错误
	WYErrorCode_CtrlParseError,	//控制包解析出错
	WYErrorCode_UploadBlockError,	//数据包解析出错
};
}


#define WY_STEP(step) WYStep_##step = step
namespace WYStep
{
	enum WYErrorStep
	{
		WYStep_None = 0,
		WYStep_CreateFile,
		WYStep_ReadFile,
		WYStep_FileSign_Beg = 1000,
		WY_STEP(1001),
		WY_STEP(1002),
		WY_STEP(1003),
		WY_STEP(1004),
		WY_STEP(1005),
		WY_STEP(1006),
		WY_STEP(1007),
		WY_STEP(1008),
		WYStep_Upload_Beg = 1000,
		WY_STEP(2000),
		WY_STEP(2001),
		WY_STEP(2002),
		WY_STEP(2003),
		WY_STEP(2004),
		WY_STEP(2005),
		WY_STEP(2006),
		WY_STEP(2007),
	};
}