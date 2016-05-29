#include "stdafx.h"
#include <MMSystem.h>
#include "TXPerfLogTime.h"
#pragma comment(lib,"Winmm.lib")
// 获取时间戳
class CGFPerfTimeStamp
{
public:
	CGFPerfTimeStamp(void)
	{
		m_bHighResolutionEnable = QueryPerformanceFrequency((LARGE_INTEGER*)&m_iFrequency);
	}

	~CGFPerfTimeStamp(void)
	{

	}

	__int64 GetPerfTimeStamp()
	{
		if (m_bHighResolutionEnable)
		{
			__int64 iCurTime = 0;
			QueryPerformanceCounter((LARGE_INTEGER*)&iCurTime);

			return (__int64)((iCurTime/(double)m_iFrequency)*1000*1000);
		}
		else
		{
			return timeGetTime() * 1000;
		}
	}
protected:
	BOOL	m_bHighResolutionEnable;
	__int64 m_iFrequency;
};

void Util::Perf::SetSingleProcess(void)
{
	DWORD dwProcess = 0;
	DWORD dwSystem = 0;

	GetProcessAffinityMask(GetCurrentProcess(), &dwProcess, &dwSystem);
	dwProcess = 1;
	SetProcessAffinityMask(GetCurrentProcess(), dwProcess);
}

__int64 Util::Perf::GetPerfTime(void)
{
	static CGFPerfTimeStamp s_timeStamp;

	return s_timeStamp.GetPerfTimeStamp();
}