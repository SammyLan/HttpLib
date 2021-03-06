#include "stdafx.h"
#include <string>


CWYStringA ByteToStringA(const BYTE* pBuffer, INT nLen)
{
	CWYStringA strResult;
	if (!pBuffer || !nLen) 
	{
		return strResult;
	}

	int i = 0;
	while (i < nLen)
	{
		strResult.AppendFormat("%02X", pBuffer[i++]);
	}
	return strResult;
}


BOOL StringToByte(const CWYString& strVal, BYTE* pBuffer, INT nLen)
{
	INT nStr = strVal.GetLength();
	if (nStr != (nLen << 1)) 
	{
		return FALSE;
	}

	BYTE b = 0;
	int i = 0;
	while (i < nStr)
	{
		BYTE tmp = 0;
		TCHAR ch = strVal[i];
		if (ch >= _T('A') && ch <= _T('F'))
		{
			tmp = (ch - _T('A')) + 10;
		}
		else if (ch >= _T('a') && ch <= _T('f'))
		{
			tmp = (ch - _T('a')) + 10;
		}
		else if (ch >= _T('0') && ch <= _T('9'))
		{
			tmp = ch - _T('0');
		}
		else
		{
			return FALSE;
		}

		if (i % 2)
		{
			b |= tmp;
			pBuffer[i / 2] = b;
			b = 0;
		}
		else
		{
			b = tmp << 4;
		}

		++i;
	}

	return TRUE;
}