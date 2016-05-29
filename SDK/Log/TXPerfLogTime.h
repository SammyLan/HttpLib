#pragma once

namespace Util
{
	namespace Perf
	{
		// 设置进程只使用单核，为了数据的准确性，最好在进程启动时设置
		void SetSingleProcess(void);
		// 获开机后流逝的时间(单位微秒)
		__int64 GetPerfTime(void);
	}
}