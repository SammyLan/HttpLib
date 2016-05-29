#pragma once

namespace WYError
{
enum WYErrorCode
{
	WYErrorCode_Begin = 1000000,
	WYErrorCode_UnkError,			//δ֪����
	WYErrorCode_NetError,			//�������
	WYErrorCode_CtrlParseError,	//���ư���������
	WYErrorCode_UploadBlockError,	//���ݰ���������
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