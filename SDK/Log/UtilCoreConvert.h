/**
@file
@brief		���ʽת����صİ�������
@version	2007-07-13 yuyu
 created
*/

#pragma once
#include <vector>
using namespace std;
namespace Util
{
	/// ��TXData��صİ�������
	/**@ingroup ov_Convert
	*/
	namespace Convert
	{

		/// Unicode תutf8�����Ƶ�һ�����ֽ������ʺ���serverЭ��ضϡ�  ..
		CWYStringA Utf8FromWSLimit(int nLimit, const wchar_t * str, int nLen=-1);

		/// Unicode ת�� utf8
		CWYStringA Utf8FromWS(const wchar_t * str, int nLen = -1);

		/// utf8 ת Unicode
		CWYStringW Utf8ToWS(const char * str, int nLen = -1);

		/// unicode ת GBK
		bool UnicodeToGBK(CWYStringA & strGBK, const wchar_t * str, int nLen = -1);

		/// GBK ת unicode.
		bool GBKToUnicode(CWYStringW & strUni, const char * str, int nLen = -1);

		/// BIG5 ת unicode.
		bool BIG5ToUnicode(CWYStringW & strUni, const char * str, int nLen = -1);

		/// ANSI(ϵͳĬ��codepage) ת unicode.
		bool AnsiToUnicode(CWYStringW & strUni, const char * str, int nLen = -1);

		/// ȫ��ת���
		bool SBCToDBC(CWYStringW & strSBC);

		/// ���תȫ��
		bool DBCToSBC(CWYStringW & strDBC);

		bool StringToIPPort(const CWYString &strSrc, CWYString &strIP, WORD &wPort);

		void StringToStringVec(const CWYString &strSrc, const CWYString &strSplider, std::vector<CWYString> &vecStringDest);

		// --- ����������ת������ -------------
		CWYStringA IntToStringA(int nValue);
		CWYStringA DWordToStringA(DWORD dwValue);
		CWYStringA Int64ToStringA(__int64 i64Value);
		CWYStringA UnsignedInt64ToStringA(unsigned __int64 ui64Value);

		CWYStringW IntToStringW(int nValue);
		CWYStringW DWordToStringW(DWORD dwValue);
		CWYStringW Int64ToStringW(__int64 n64Value);
		CWYStringW UnsignedInt64ToStringW(unsigned __int64 n64Value);

		bool StringToIntA(const char* strValue, int & nValue);
		bool StringToDWordA(const char* strValue, DWORD & dwValue);
		bool StringToInt64A(const char* strValue, __int64 & i64Value);
		bool StringToUnsignedInt64A(const char* strValue, unsigned __int64 & ui64Value);

		bool StringToIntW(const wchar_t * strValue, int & nValue);
		bool StringToDWordW(const wchar_t * strValue, DWORD & dwValue);
		bool StringToInt64W(const wchar_t * strValue, __int64 & i64Value);
		bool StringToUnsignedInt64W(const wchar_t * strValue, unsigned __int64 & ui64Value);

		//�������жϺ���
		bool __stdcall IsTextUtf8(const void * ptr_, int nLen);

		void SplitCmdString(const CWYString &str, std::vector<CWYString> &vecStr);

#define TXA2W(x) (CStringW(tagUTF8(), (x)).GetString())
#define TXA2B(x) (CComBSTR(TXA2W(x)))
#define TXW2A(x) (CWYStringA(tagUTF8(), (x)).GetString())
	}
}
