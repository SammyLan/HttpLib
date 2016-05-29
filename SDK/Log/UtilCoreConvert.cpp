#include "stdafx.h"
#include "UtilCoreConvert.h"

#ifndef WIN32
	#include <iconv.h>
#endif

namespace BinHelper
{
	struct _0000{ enum{value = 0}; };
	struct _0001{ enum{value = 1}; };
	struct _0010{ enum{value = 2}; };
	struct _0011{ enum{value = 3}; };
	struct _0100{ enum{value = 4}; };
	struct _0101{ enum{value = 5}; };
	struct _0110{ enum{value = 6}; };
	struct _0111{ enum{value = 7}; };

	struct _1000{ enum{value = 0+8}; };
	struct _1001{ enum{value = 1+8}; };
	struct _1010{ enum{value = 2+8}; };
	struct _1011{ enum{value = 3+8}; };
	struct _1100{ enum{value = 4+8}; };
	struct _1101{ enum{value = 5+8}; };
	struct _1110{ enum{value = 6+8}; };
	struct _1111{ enum{value = 7+8}; };
}

#define BIN__(a) (BinHelper::a::value)
#define BIN_(a) BIN__(_##a)
#define BIN(a,b) ((BIN_(a)<<4) + BIN_(b))

static inline bool Match_(BYTE * ptr, int len, int o, BYTE v1, BYTE v2)
{
	return o<len && (ptr[o] & v1) == v2;
}


CWYStringA Util::Convert::Utf8FromWS(const wchar_t * str, int len)
{
	return Utf8FromWSLimit(0x7fffffff, str, len);
}

CWYStringA Util::Convert::Utf8FromWSLimit(int limit, const wchar_t * str, int len)
{
	if(len<0) len = _tcslen(str);
	CWYStringA sr;
	if (limit <= 0 || len == 0)
	{
		return sr;
	}
	int utf8len = 0;
	for (int i=0; i<len; ++i)
	{
		int marki = i;
		unsigned short us = str[i];
		if (us == 0)
		{
			utf8len += 2;
		}
		else if (us <= 0x7f)
		{
			utf8len += 1;
		}
		else if (us <= 0x7ff)
		{
			utf8len += 2;
		}
		else if (0xd800 <= us && us <= 0xdbff && i+1<len)
		{
			unsigned short ul = str[i+1];
			if (0xdc00 <= ul && ul <= 0xdfff)
			{
				++i;
				utf8len += 4;
			}
			else
			{
				utf8len += 3;
			}
		}
		else
		{
			utf8len += 3;
		}
		if (utf8len > limit)
		{
			len = marki;
			break;
		}
	}

	PBYTE pc = (PBYTE)sr.GetBuffer(utf8len+1);
	for (int i=0; i<len; ++i)
	{
		unsigned short us = str[i];
		if (us == 0)
		{
			*pc ++ = 0xc0;
			*pc ++ = 0x80;
		}
		else if (us <= 0x7f)
		{
			*pc ++ = (char)us;
		}
		else if (us <= 0x7ff)
		{
			*pc ++ = 0xc0 | (us >> 6);
			*pc ++ = 0x80 | (us & 0x3f);
		}
		else if(0xd800 <= us && us <= 0xdbff && i+1<len)
		{
			unsigned short ul = str[i+1];
			if (0xdc00 <= ul && ul <= 0xdfff)
			{
				++i;
				UINT bc = (us-0xD800)*0x400 + (ul-0xDC00) + 0x10000;
				*pc ++ = (BYTE)(0xf0 | ((bc >> 18) & 0x07));
				*pc ++ = (BYTE)(0x80 | ((bc >> 12) & 0x3f));
				*pc ++ = (BYTE)(0x80 | ((bc >>  6) & 0x3f));
				*pc ++ = (BYTE)(0x80 | ((bc      ) & 0x3f));
			}
			else
			{
				*pc ++ = (BYTE) (0xe0 | ((us >> 12) & 0x0f));
				*pc ++ = (BYTE) (0x80 | ((us >>  6) & 0x3f));
				*pc ++ = (BYTE) (0x80 | ((us      ) & 0x3f));
			}
		}
		else
		{
			*pc ++ = (BYTE) (0xe0 | ((us >> 12) & 0x0f));
			*pc ++ = (BYTE) (0x80 | ((us >>  6) & 0x3f));
			*pc ++ = (BYTE) (0x80 | ((us      ) & 0x3f));
		}
	}
	* pc++ = 0;
	sr.ReleaseBuffer();
	return sr;
}

bool Util::Convert::BIG5ToUnicode(CWYStringW & strUni, const char * str, int len /* = -1 */)
{
#ifdef WIN32
	CWYStringW sr;
	if (len < 0) len = (int)strlen(str);
	if (len <= 0)
	{
		strUni = sr;
		return true;
	}

	LPWSTR buf = sr.GetBuffer(len);
	bool bEn = true;
	int i = 0;
	for (; i < len; ++i)
	{
		if (((unsigned char)str[i]) >= 0x80)
		{
			bEn = false;
			break;
		}
		buf[i] = str[i];
	}

	if (bEn)
	{
		sr.ReleaseBuffer(len);
		strUni = sr;
		return true;
	}
	
	int g = MultiByteToWideChar(950, 0, str+i, len-i, buf+i, len-i);
	if (g <= 0)
	{
		g = 0;

		wchar_t * buf2 = buf;
		for (; i<len; ++i)
		{
			if (((unsigned char)str[i]) >= 0x80)
			{
				++i;
				buf2[g++] = '?';
			}
			else
			{
				buf2[g++] = str[i];
			}
		}
	}

	sr.ReleaseBuffer(i + g);
	strUni = sr;

	return g > 0; //g>0表明转换成功。

#else
	CWYStringW sr;
	if (len < 0) len = (int)strlen(str);
	if (len <= 0)
	{
		strUni = sr;
		return true;
	}
#ifdef __LINUX__
	iconv_t cd = iconv_open ("UCS-4LE", "GBK");
#else	
	iconv_t cd = iconv_open ("UCS-4-INTERNAL", "GBK");
#endif
	if (-1 == (int)cd)
	{
		//	printf("iconv_open Error: $d [%s]\n", strerror(errno));
		return false;
	}	
	char *outTmp=(char *)sr.GetBuffer( (len+1) * sizeof(wchar_t) );
	size_t outbytesleft = sizeof(wchar_t) * (len + 1);
	size_t inbytesleft = len ;

	while (inbytesleft > 0) 
	{
		int ee = iconv(cd, (char**)&str, &inbytesleft, &outTmp, &outbytesleft); 
		if(0 != ee)
		{
			//printf("non reversible convert charactar  is: %d \n",ee);
		}

		if (ee == (size_t)(-1)) 
		{
			LogFinal(L"GBKToUnicode",L"iconv Error: $d [%s]\n", (LPCTSTR)strerror(errno));
			return false;
		}
	}
	iconv_close(cd);
	sr.ReleaseBuffer( (len+1) * sizeof(wchar_t) - outbytesleft );
	strUni = sr;
	return true;
#endif
}

bool Util::Convert::GBKToUnicode(CWYStringW & strUni, const char * str, int len /* = -1 */)
{
#ifdef WIN32
	CWYStringW sr;
	if (len < 0) len = (int)strlen(str);
	if (len <= 0)
	{
		strUni = sr;
		return true;
	}

	LPWSTR buf = sr.GetBuffer(len);
	bool bEn = true;
	int i = 0;
	for (; i < len; ++i)
	{
		if (((unsigned char)str[i]) >= 0x80)
		{
			bEn = false;
			break;
		}
		buf[i] = str[i];
	}

	if (bEn)
	{
		sr.ReleaseBuffer(len);
		strUni = sr;
		return true;
	}
	else
	{
		int g = MultiByteToWideChar(936, 0, str+i, len-i, buf+i, len-i);
		if (g <= 0)
		{
			g = 0;
			wchar_t * buf2 = buf;
			for (; i<len; ++i)
			{
				if (((unsigned char)str[i]) >= 0x80)
				{
					++i;
					buf2[g++] = '?';
				}
				else
				{
					buf2[g++] = str[i];
				}
			}
		}
		sr.ReleaseBuffer(i + g);
		strUni = sr;
		return g > 0; //g>0表明转换成功。
	}
#else
	CWYStringW sr;
	if (len < 0) len = (int)strlen(str);
	if (len <= 0)
	{
		strUni = sr;
		return true;
	}
	
#ifdef __LINUX__
	iconv_t cd = iconv_open ("UCS-4LE", "GBK");
#else		
	iconv_t cd = iconv_open ("UCS-4-INTERNAL", "GBK");
#endif
	if (-1 == (int)cd)
	{
		//	printf("iconv_open Error: $d [%s]\n", strerror(errno));
		return false;
	}	
	char *outTmp=(char *)sr.GetBuffer( (len+1) * sizeof(wchar_t) );
	size_t outbytesleft = sizeof(wchar_t) * (len + 1);
	size_t inbytesleft = len ;
	
	while (inbytesleft > 0) 
	{
		int ee = iconv(cd, (char**)&str, &inbytesleft, &outTmp, &outbytesleft); 
		if(0 != ee)
		{
			//printf("non reversible convert charactar  is: %d \n",ee);
		}
		
		if (ee == (size_t)(-1)) 
		{
			LogFinal(L"GBKToUnicode",L"iconv Error: $d [%s]\n", (LPCTSTR)strerror(errno));
			return false;
		}
	}
	iconv_close(cd);
	sr.ReleaseBuffer( (len+1) * sizeof(wchar_t) - outbytesleft );
	strUni = sr;
	return true;
#endif
}

bool Util::Convert::UnicodeToGBK(CWYStringA & strGBK, const wchar_t * str, int len /* = -1 */)
{
#ifdef WIN32
	CWYStringA sr;
	if (len < 0) len = wcslen(str);
	if (len <= 0)
	{
		strGBK = sr;
		return true;
	}
	bool bEn = true;
	char * buf = sr.GetBuffer(len*2);
	int i=0;
	for (; i<len; ++i)
	{
		if ( ((unsigned short)str[i]) >= 0x80)
		{
			bEn = false;
			break;
		}
		buf[i] = (char)str[i];
	}
	if (bEn)
	{
		sr.ReleaseBuffer(len);
		strGBK = sr;
		return true;
	}
	else
	{
		BOOL bUseDef = FALSE;
		int g = WideCharToMultiByte(936, 0, str+i, len-i, buf+i, len*2-i, NULL, &bUseDef);
		if (g <= 0)
		{
			g = 0;
			char * buf2 = buf;
			for (; i<len; ++i)
			{
				if (str[i] >= 0x80)
				{
					buf2[g++] = '?';
				}
				else
				{
					buf2[g++] = (char)str[i];
				}
			}
		}
		sr.ReleaseBuffer(i+g);
		strGBK = sr;
		return g>0;
	}
#else
	CWYStringA  sr;
	if (len < 0) len = (int)wcslen(str);
	if (len <= 0)
	{
		strGBK = sr;
		return true;
	}
	const char *inBuf=(const char *)str;
#ifdef __LINUX__
	iconv_t cd = iconv_open ("GBK", "UCS-4LE");
#else		
	iconv_t cd = iconv_open ("GBK", "UCS-4-INTERNAL");
#endif
	if (-1 == (int)cd)
	{
		//	printf("iconv_open Error: $d [%s]\n", strerror(errno));
		return false;
	}	
	char *outTmp=(char *)sr.GetBuffer( (len+1) * sizeof(wchar_t) );
	size_t outbytesleft = sizeof(wchar_t) * (len + 1);
	size_t inbytesleft = len * sizeof(wchar_t);
	
	while (inbytesleft > 0) 
	{
		int ee = iconv(cd, (char**)&inBuf, &inbytesleft, &outTmp, &outbytesleft);
		if(0 != ee)
		{
			//printf("non reversible convert charactar  is: %d \n",ee);
		}
		
		if (ee == (size_t)(-1)) 
		{
			//printf("iconv Error: $d [%s]\n", strerror(errno));
			return false;
		}
	}
	iconv_close(cd);
	sr.ReleaseBuffer( (len+1) * sizeof(wchar_t) - outbytesleft);
	strGBK = sr;
	return true;
#endif
}

#ifdef WIN32
bool Util::Convert::AnsiToUnicode(CWYStringW & strUni, const char * str, int len /* = -1 */)
{
	CWYStringW sr;
	if (len < 0) len = (int)strlen(str);
	if (len <= 0)
	{
		strUni = sr;
		return true;
	}

	LPWSTR buf = sr.GetBuffer(len);
	bool bEn = true;
	int i = 0;
	for (; i < len; ++i)
	{
		if (((unsigned char)str[i]) >= 0x80)
		{
			bEn = false;
			break;
		}
		buf[i] = str[i];
	}

	if (bEn)
	{
		sr.ReleaseBuffer(len);
		strUni = sr;
		return true;
	}
	else
	{
		int g = MultiByteToWideChar(CP_ACP, 0, str+i, len-i, buf+i, len-i);
		if (g <= 0)
		{
			g = 0;
			wchar_t * buf2 = buf;
			for (; i<len; ++i)
			{
				if (((unsigned char)str[i]) >= 0x80)
				{
					++i;
					buf2[g++] = '?';
				}
				else
				{
					buf2[g++] = str[i];
				}
			}
		}
		sr.ReleaseBuffer(i + g);
		strUni = sr;
		return g > 0; //g>0表明转换成功。
	}
}
#endif

CWYStringA Util::Convert::IntToStringA(int iValue)
{
	CWYStringA str;
	str.Format("%d", iValue);
	return str;
}
CWYStringA Util::Convert::DWordToStringA(DWORD dwValue)
{
	CWYStringA str;
	str.Format("%lu", dwValue);
	return str;
}
CWYStringA Util::Convert::Int64ToStringA(__int64 i64Value)
{
	CWYStringA str;
	str.Format("%I64d", i64Value);
	return str;
}
CWYStringA Util::Convert::UnsignedInt64ToStringA(unsigned __int64 ui64Value)
{
	CWYStringA str;
	str.Format("%I64u", ui64Value);
	return str;
}

CWYStringW Util::Convert::IntToStringW(int iValue)
{
	CWYStringW str;
	str.Format(L"%d", iValue);
	return str;
}
CWYStringW Util::Convert::DWordToStringW(DWORD dwValue)
{
	CWYStringW str;
	str.Format(L"%lu", dwValue);
	return str;
}
CWYStringW Util::Convert::Int64ToStringW(__int64 iValue)
{
	CWYStringW str;
	str.Format(L"%I64d", iValue);
	return str;
}
CWYStringW Util::Convert::UnsignedInt64ToStringW(unsigned __int64 iValue)
{
	CWYStringW str;
	str.Format(L"%I64u", iValue);
	return str;
}

bool Util::Convert::StringToIntA(const char* strValue, int & iValue)
{
	iValue = 0;
	bool br = true;
	if (!strValue || !*strValue)
	{
		return false;
	}
	while (isspace((int)*strValue))
	{
		strValue ++;
	}
	if (*strValue == '+' || *strValue == '-')
	{
		br = (*strValue++ == '+');
	}
	bool bOK = true;
	if (strValue[0] == '0' && (strValue[1]|0x20)=='x')
	{
		strValue += 2;
		for (;;)
		{
			TCHAR ch = *strValue;
			int iValue2 = 0;
			if (ch >= '0' && ch <= '9')	iValue2 = iValue*16 + ch -'0';
			else if (ch>='a' && ch<='f') iValue2 = iValue*16 + ch -'a'+10;
			else if (ch>='A' && ch<='F') iValue2 = iValue*16 + ch -'A'+10;
			else break;
			if (iValue2 < 0 || iValue >= 134217728)	bOK = false;
			iValue = iValue2;
			++strValue;
		}
	}
	else
	{
		while (*strValue >= '0' && *strValue <= '9')
		{
			int iValue2 = iValue * 10 + *strValue++ -'0';
			if (iValue2 < 0 || iValue > 214748364) bOK = false;
			iValue = iValue2;
		}
	}
	if (!br) iValue = -iValue;
	while (*strValue && isspace((BYTE)*strValue)) ++strValue;
	return bOK && strValue[0] == 0;
}
bool Util::Convert::StringToDWordA(const char* strValue, DWORD & dwValue)
{
	dwValue = 0;
	if (!strValue || !*strValue)
	{
		return false;
	}
	while (isspace((int)*strValue))
	{
		strValue ++;
	}
	if (*strValue == '+')
	{
		strValue ++;
	}
	bool bOK = true;
	if (strValue[0] == '0' && (strValue[1]|0x20)=='x')
	{
		strValue += 2;
		for (;;)
		{
			TCHAR ch = *strValue;
			DWORD dwValue2 = 0;
			if (ch >= '0' && ch <= '9')	dwValue2 = dwValue*16 + ch -'0';
			else if (ch>='a' && ch<='f') dwValue2 = dwValue*16 + ch -'a'+10;
			else if (ch>='A' && ch<='F') dwValue2 = dwValue*16 + ch -'A'+10;
			else break;
			if (dwValue2 < dwValue || dwValue >= 268435456) bOK = false;
			dwValue = dwValue2;
			++strValue;
		}
	}
	else
	{
		while (*strValue >= '0' && *strValue <= '9')
		{
			DWORD dwValue2 = dwValue * 10 + *strValue++ -'0';
			if (dwValue2 < dwValue || dwValue > 429496729) bOK = false;
			dwValue = dwValue2;
		}
	}
	while (*strValue && isspace((BYTE)*strValue)) ++strValue;
	return bOK && strValue[0] == 0;
}
bool Util::Convert::StringToInt64A(const char* strValue, __int64 & i64Value)
{
	i64Value = 0;
	bool br = true;
	if (!strValue || !*strValue)
	{
		return false;
	}
	while (isspace((int)*strValue))
	{
		strValue ++;
	}
	if (*strValue == '+' || *strValue == '-')
	{
		br = (*strValue++ == '+');
	}
	bool bOK = true;
	if (strValue[0] == '0' && (strValue[1]|0x20)=='x')
	{
		strValue += 2;
		for (;;)
		{
			TCHAR ch = *strValue;
			__int64 i64Value2 = 0;
			if (ch >= '0' && ch <= '9')	i64Value2 = i64Value*16 + ch -'0';
			else if (ch>='a' && ch<='f') i64Value2 = i64Value*16 + ch -'a'+10;
			else if (ch>='A' && ch<='F') i64Value2 = i64Value*16 + ch -'A'+10;
			else break;
			if (i64Value2 < 0 || i64Value >= 576460752303423488ULL) bOK = false;
			i64Value = i64Value2;
			++strValue;
		}
	}
	else
	{
		while (*strValue >= '0' && *strValue <= '9')
		{
			__int64 i64Value2 = i64Value * 10 + *strValue++ -'0';
			if (i64Value2 < 0 || i64Value > 922337203685477580ULL) bOK = false;
			
			i64Value = i64Value2;
		}
	}
	if (!br) i64Value = -i64Value;
	while (*strValue && isspace((BYTE)*strValue)) ++strValue;
	return bOK && strValue[0] == 0;
}
bool Util::Convert::StringToUnsignedInt64A(const char* strValue, unsigned __int64 & ui64Value)
{
	ui64Value = 0;
	if (!strValue || !*strValue)
	{
		return false;
	}
	while (isspace((int)*strValue))
	{
		strValue ++;
	}
	if (*strValue == '+')
	{
		strValue ++;
	}
	bool bOK = true;
	if (strValue[0] == '0' && (strValue[1]|0x20)=='x')
	{
		strValue += 2;
		for (;;)
		{
			TCHAR ch = *strValue;
			unsigned __int64 ui64Value2 = 0;
			if (ch >= '0' && ch <= '9')	ui64Value2 = ui64Value*16 + ch -'0';
			else if (ch>='a' && ch<='f') ui64Value2 = ui64Value*16 + ch -'a'+10;
			else if (ch>='A' && ch<='F') ui64Value2 = ui64Value*16 + ch -'A'+10;
			else break;
			if (ui64Value2 < ui64Value || ui64Value >= 1152921504606846976ULL) bOK = false;
			ui64Value = ui64Value2;
			++strValue;
		}
	}
	else
	{
		while (*strValue >= '0' && *strValue <= '9')
		{
			unsigned __int64 ui64Value2 = ui64Value * 10 + *strValue++ -'0';
			if (ui64Value2 < ui64Value || ui64Value > 1844674407370955161ULL) bOK = false;
			ui64Value = ui64Value2;
		}
	}
	while (*strValue && isspace((BYTE)*strValue)) ++strValue;
	return bOK && strValue[0] == 0;
}

bool Util::Convert::StringToIntW(const wchar_t* strValue, int & iValue)
{
	iValue = 0;
	bool br = true;
	if (!strValue || !*strValue)
	{
		return false;
	}
	while (iswspace(*strValue))
	{
		strValue ++;
	}
	if (*strValue == '+' || *strValue == '-')
	{
		br = (*strValue++ == '+');
	}
	bool bOK = true;
	if (strValue[0] == '0' && (strValue[1]|0x20)=='x')
	{
		strValue += 2;
		for (;;)
		{
			TCHAR ch = *strValue;
			int iValue2 = 0;
			if (ch >= '0' && ch <= '9')	iValue2 = iValue*16 + ch -'0';
			else if (ch>='a' && ch<='f') iValue2 = iValue*16 + ch -'a'+10;
			else if (ch>='A' && ch<='F') iValue2 = iValue*16 + ch -'A'+10;
			else break;
			if (iValue2 < 0 || iValue >= 134217728)	bOK = false;
			iValue = iValue2;
			++strValue;
		}
	}
	else
	{
		while (*strValue >= '0' && *strValue <= '9')
		{
			int iValue2 = iValue * 10 + *strValue++ -'0';
			if (iValue2 < 0 || iValue > 214748364) bOK = false;
			iValue = iValue2;
		}
	}
	if (!br) iValue = -iValue;
	while (*strValue && isspace((BYTE)*strValue)) ++strValue;
	return bOK && strValue[0] == 0;
}

bool Util::Convert::StringToDWordW(const wchar_t* strValue, DWORD & dwValue)
{
	dwValue = 0;
	if (!strValue || !*strValue)
	{
		return false;
	}
	while (iswspace(*strValue))
	{
		strValue ++;
	}
	if (*strValue == '+')
	{
		strValue ++;
	}
	bool bOK = true;
	if (strValue[0] == '0' && (strValue[1]|0x20)=='x')
	{
		strValue += 2;
		for (;;)
		{
			TCHAR ch = *strValue;
			DWORD dwValue2 = 0;
			if (ch >= '0' && ch <= '9')	dwValue2 = dwValue*16 + ch -'0';
			else if (ch>='a' && ch<='f') dwValue2 = dwValue*16 + ch -'a'+10;
			else if (ch>='A' && ch<='F') dwValue2 = dwValue*16 + ch -'A'+10;
			else break;
			if (dwValue2 < dwValue || dwValue >= 268435456) bOK = false;
			dwValue = dwValue2;
			++strValue;
		}
	}
	else
	{
		while (*strValue >= '0' && *strValue <= '9')
		{
			DWORD dwValue2 = dwValue * 10 + *strValue++ -'0';
			if (dwValue2 < dwValue || dwValue > 429496729) bOK = false;
			dwValue = dwValue2;
		}
	}
	while (*strValue && isspace((BYTE)*strValue)) ++strValue;
	return bOK && strValue[0] == 0;
}
bool Util::Convert::StringToInt64W(const wchar_t* strValue, __int64 & i64Value)
{
	i64Value = 0;
	bool br = true;
	if (!strValue || !*strValue)
	{
		return false;
	}
	while (iswspace(*strValue))
	{
		strValue ++;
	}
	if (*strValue == '+' || *strValue == '-')
	{
		br = (*strValue++ == '+');
	}
	bool bOK = true;
	if (strValue[0] == '0' && (strValue[1]|0x20)=='x')
	{
		strValue += 2;
		for (;;)
		{
			TCHAR ch = *strValue;
			__int64 i64Value2 = 0; 
			if (ch >= '0' && ch <= '9')	i64Value2 = i64Value*16 + ch -'0';
			else if (ch>='a' && ch<='f') i64Value2 = i64Value*16 + ch -'a'+10;
			else if (ch>='A' && ch<='F') i64Value2 = i64Value*16 + ch -'A'+10;
			else break;
			if (i64Value2 < 0 || i64Value >= 1844674407370955161ULL) bOK = false;
			i64Value = i64Value2;
			++strValue;
		}
	}
	else
	{
		while (*strValue >= '0' && *strValue <= '9')
		{
			__int64 i64Value2 = i64Value * 10 + *strValue++ -'0';
			if (i64Value2 < 0 || i64Value > 576460752303423488ULL) bOK = false;
			i64Value = i64Value2;
		}
	}
	if (!br) i64Value = -i64Value;
	while (*strValue && isspace((BYTE)*strValue)) ++strValue;
	return bOK && strValue[0] == 0;
}
bool Util::Convert::StringToUnsignedInt64W(const wchar_t* strValue, unsigned __int64 & ui64Value)
{
	ui64Value = 0;
	if (!strValue || !*strValue)
	{
		return false;
	}
	while (iswspace(*strValue))
	{
		strValue ++;
	}
	if (*strValue == '+')
	{
		strValue ++;
	}
	bool bOK = true;
	if (strValue[0] == '0' && (strValue[1]|0x20)=='x')
	{
		strValue += 2;
		for (;;)
		{
			TCHAR ch = *strValue;
			unsigned __int64 ui64Value2 = 0;
			if (ch >= '0' && ch <= '9')	ui64Value2 = ui64Value*16 + ch -'0';
			else if (ch>='a' && ch<='f') ui64Value2 = ui64Value*16 + ch -'a'+10;
			else if (ch>='A' && ch<='F') ui64Value2 = ui64Value*16 + ch -'A'+10;
			else break;
			if (ui64Value2 < ui64Value || ui64Value >= 1152921504606846976ULL) bOK = false;
			ui64Value = ui64Value2;
			++strValue;
		}
	}
	else
	{
		while (*strValue >= '0' && *strValue <= '9')
		{
			unsigned __int64 ui64Value2 = ui64Value * 10 + *strValue++ -'0';
			if (ui64Value2 < ui64Value || ui64Value > 1844674407370955161ULL) bOK = false;
			ui64Value = ui64Value2;
		}
	}
	while (*strValue && isspace((BYTE)*strValue)) ++strValue;
	return bOK && strValue[0] == 0;
}

bool Util::Convert::SBCToDBC(CWYStringW & strSBC)
{
	const WCHAR cAt = 183;
	const WCHAR cAt2 = 9678;
	const WCHAR cSpace = 12288;
	const WCHAR cPunc_Dot = 12290;
	const WCHAR cBegin = 65280;
	const WCHAR cEnd = 65375;

	bool bRet = false;

	for (INT nLoc = 0; nLoc < strSBC.GetLength(); nLoc++)
	{
		if ( (strSBC[nLoc] == cAt) || (strSBC[nLoc] == cAt2) )
		{
			strSBC.SetAt(nLoc, L'@');
			bRet = true;
		}
		else if (strSBC[nLoc] == cPunc_Dot)
		{
			strSBC.SetAt(nLoc, L'.');
			bRet = true;
		}
		else if (strSBC[nLoc] == cSpace)
		{
			strSBC.SetAt(nLoc, strSBC[nLoc]-12256);
			bRet = true;
		}
		else if (strSBC[nLoc] > cBegin && strSBC[nLoc] < cEnd)
		{
			strSBC.SetAt(nLoc, strSBC[nLoc]-65248);
			bRet = true;
		}
	}

	return bRet;
}

bool Util::Convert::DBCToSBC(CWYStringW & strDBC)
{
	const WCHAR cAt = 183;
	const WCHAR cAt2 = 9678;
	const WCHAR cSpace = 12288;
	const WCHAR cPunc_Dot = 12290;
	const WCHAR cBegin = 65280 - 65248;
	const WCHAR cEnd = 65375 - 65248;

	bool bRet = false;

	for (INT nLoc = 0; nLoc < strDBC.GetLength(); nLoc++)
	{
		//if ( (strDBC[nLoc] == cAt) || (strDBC[nLoc] == cAt2) )
		//{
		//	strSBC.SetAt(nLoc, L'@');
		//	bRet = true;
		//}
		if (strDBC[nLoc] == L'.')
		{
			strDBC.SetAt(nLoc, cPunc_Dot);
			bRet = true;
		}
		else if (strDBC[nLoc] == L' ')
		{
			strDBC.SetAt(nLoc, strDBC[nLoc] + 12256);
			bRet = true;
		}
		else if (strDBC[nLoc] > cBegin && strDBC[nLoc] < cEnd)
		{
			strDBC.SetAt(nLoc, strDBC[nLoc] + 65248);
			bRet = true;
		}
	}

	return bRet;
}
CWYStringW Util::Convert::Utf8ToWS(const char * str, int len)
{
	CWYStringW sw;
	if (str == 0) return sw;
	if (len <  0) len = (int)strlen(str);
	if (len == 0) return sw;
#ifdef WIN32
	int nLength = MultiByteToWideChar(CP_UTF8,0,str,len,0,0);
	MultiByteToWideChar(CP_UTF8,0,str,len,sw.GetBuffer(nLength+1),nLength);
	sw.ReleaseBuffer(nLength);
#else
#ifdef __LINUX__
	iconv_t cd = iconv_open ("UCS-4LE", "UTF-8");
#else	
	iconv_t cd = iconv_open ("UCS-4-INTERNAL", "UTF-8");
#endif
	int ee = errno;
	char *p = strerror(ee);
	char *outbuf = (char*)calloc(len +1, sizeof(wchar_t));
	char *tmpBuf = outbuf;
	*outbuf = 0;
	size_t outbytesleft = sizeof(wchar_t) * (len + 1);
	size_t inbytesleft = len;
	while (inbytesleft > 0) 
	{
		ee = iconv(cd, (char**)&str, &inbytesleft, &tmpBuf, &outbytesleft);
		if (ee == (size_t)(-1)) 
		{
			ee = errno;
			p = strerror(ee);
			break;
		}
	}
	//iconv(cd, &str, &inbytesleft, &outbuf, &outbytesleft);
	iconv_close(cd);
	
	sw = (wchar_t*)outbuf;
	free(outbuf);
#endif
	return sw;
}
bool __stdcall Util::Convert::IsTextUtf8(const void * ptr_, int nLen)
{
	BYTE * ptr = (BYTE*)ptr_;
	for (int i=0; i<nLen; )
	{
		if (ptr[i] == 0) 
		{
			return false;
		}
		if (ptr[i] < 0x80)
		{
			++ i;
		}
		else if (
			Match_(ptr, nLen, i+0, BIN(1110, 0000), BIN(1100, 0000)) &&
			Match_(ptr, nLen, i+1, BIN(1100, 0000), BIN(1000, 0000)) 
			)
		{
			i += 2;
		}
		else if (
			Match_(ptr, nLen, i+0, BIN(1111, 0000), BIN(1110, 0000)) &&
			Match_(ptr, nLen, i+1, BIN(1100, 0000), BIN(1000, 0000)) &&
			Match_(ptr, nLen, i+2, BIN(1100, 0000), BIN(1000, 0000)) 
			)
		{
			i += 3;
		}
		else if (
			Match_(ptr, nLen, i+0, BIN(1111, 1000), BIN(1111, 0000)) &&
			Match_(ptr, nLen, i+1, BIN(1100, 0000), BIN(1000, 0000)) &&
			Match_(ptr, nLen, i+2, BIN(1100, 0000), BIN(1000, 0000)) &&
			Match_(ptr, nLen, i+3, BIN(1100, 0000), BIN(1000, 0000)) 
			)
		{
			i += 4;
		}
		else
		{
			return false;
		}
	}
	return true;
}


bool Util::Convert::StringToIPPort(const CWYString &strSrc, CWYString &strIP, WORD &wPort)
{
	int nPos = strSrc.Find(_T(':'));

	if( -1 == nPos )
	{
		return FALSE;
	}

	strIP = strSrc.Mid(0,nPos);
	DWORD dwPort = 0;
	CWYString strPort = strSrc.Right(strSrc.GetLength()-nPos-1);
	StringToDWordW( strPort,dwPort);
	wPort = (WORD)dwPort;

	if( 0 == wPort || dwPort >= 65535 )
	{
		return FALSE;
	}

	return TRUE;
}

void Util::Convert::StringToStringVec(const CWYString &strSrc, const CWYString &strSplider, vector<CWYString> &vecStringDest)
{
	vecStringDest.clear();

	if ( strSrc.IsEmpty() )
	{
		return;
	}

	int nSpliderLen = strSplider.GetLength();
	int nLastPos = 0;
	int nPos = -1;
	while ( -1 != (nPos = strSrc.Find(strSplider, nLastPos)) )
	{
		CWYString strTemp = strSrc.Mid(nLastPos, nPos - nLastPos);
		vecStringDest.push_back(strTemp);
		nLastPos = nPos + nSpliderLen;
	}

	CWYString strTemp = strSrc.Mid(nLastPos);
	vecStringDest.push_back(strTemp);
}

void Util::Convert::SplitCmdString(const CWYString &str, std::vector<CWYString> &vecStr)
{
	BOOL bInQuotation = FALSE;
	int nBeginChar	= 0;
	int nEndChar	= str.GetLength() - 1;

	for (int i = 0; i < str.GetLength(); )
	{
		if (str[i] == _T('\"'))
		{
			if (!bInQuotation &&	// DEBUG: "arg1""arg2" 被解析成arg2，因""被优先于InQuotation处理
				str[i+1] == _T('\"'))
			{
				i = i + 2;
				nBeginChar = i;	// DEBUG: arg1 "" arg2 被解析成3参数，由于beginChar没有相应移位导致
				continue;
			}

			if (bInQuotation)
			{
				nEndChar	= i - 1;
				bInQuotation = FALSE;
				vecStr.push_back(str.Mid(nBeginChar, nEndChar - nBeginChar + 1));
				nBeginChar	= i + 1;
			}
			else
			{
				if ((i == 0) || (str[i-1] == _T(' ')))
				{
					nBeginChar	= i + 1;
					bInQuotation= TRUE;
				}
			}
			++i;
		}
		else if (str[i] == _T(' ') || str[i] == _T('\t'))
		{
			if (bInQuotation)
			{
				++i;
				continue;
			}

			nEndChar	= i - 1;
			if (nBeginChar <= nEndChar)
			{
				vecStr.push_back(str.Mid(nBeginChar, nEndChar - nBeginChar + 1));
			}
			while ((i < str.GetLength()) && (str[i] == _T(' ') || str[i] == _T('\t')))
			{
				++i;
			}
			nBeginChar = i;
		}
		else
		{
			++i;
		}
	}

	nEndChar = str.GetLength() - 1;

	if (nBeginChar <= nEndChar)
	{
		CWYString strTemp = str.Mid(nBeginChar, nEndChar - nBeginChar + 1);
		strTemp.TrimRight();
		if (!strTemp.IsEmpty())
		{
			vecStr.push_back(strTemp);
		}
	}
}