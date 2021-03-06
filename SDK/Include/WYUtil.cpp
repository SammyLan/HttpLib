#include "stdafx.h"
#include "WYUtil.h"
namespace Util
{
	namespace Time
	{
		typedef ULONGLONG (* TGetTimeSinceStarted)();

		ULONGLONG GetTimeSinceStartedImpl()
		{
			return GetTickCount();
		}

		TGetTimeSinceStarted InitGetTickCount64()
		{
			Util::Time::TGetTimeSinceStarted func = (Util::Time::TGetTimeSinceStarted)::GetProcAddress(GetModuleHandle(_T("Kernel32.dll")), "GetTickCount64");
			if (func == NULL)
			{
				func = &GetTimeSinceStartedImpl;
			}
			return func;
		}

		TGetTimeSinceStarted s_GetTimeSinceStarted = InitGetTickCount64();

		ULONGLONG TXGetTickCount()
		{
			return s_GetTimeSinceStarted();
		}
	}
	namespace Convert
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8ToUnicode;
	}
};