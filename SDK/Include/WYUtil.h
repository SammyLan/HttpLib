#pragma once

namespace Util
{
	namespace Time
	{
		ULONGLONG TXGetTickCount();
	}

	namespace Encode
	{
		CString MakeMd5(void *pData,DWORD const dwSize);
	}
	namespace Convert
	{
		inline CString ConvertFromUtf8(LPCSTR str)
		{
			return LPCTSTR(CA2W(str, CP_UTF8));
		}
		inline CStringA ConvertToUtf8(LPCTSTR str)
		{
			return (LPCSTR)CW2A(str, CP_UTF8);
		}
	}
};