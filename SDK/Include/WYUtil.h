#pragma once

namespace Util
{
	namespace Time
	{
		ULONGLONG TXGetTickCount();
	}

	namespace Encode
	{
		CWYString MakeMd5(void *pData,DWORD const dwSize);
	}
	namespace Convert
	{
		inline CWYString ConvertFromUtf8(LPCSTR str)
		{
			return LPCTSTR(CA2W(str, CP_UTF8));
		}
		inline CWYStringA ConvertToUtf8(LPCTSTR str)
		{
			return (LPCSTR)CW2A(str, CP_UTF8);
		}
	}
};