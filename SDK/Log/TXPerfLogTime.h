#pragma once

namespace Util
{
	namespace Perf
	{
		// ���ý���ֻʹ�õ��ˣ�Ϊ�����ݵ�׼ȷ�ԣ�����ڽ�������ʱ����
		void SetSingleProcess(void);
		// �񿪻������ŵ�ʱ��(��λ΢��)
		__int64 GetPerfTime(void);
	}
}