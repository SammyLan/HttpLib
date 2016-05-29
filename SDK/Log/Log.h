/** 
@file
@brief TXLog ʵ�ֵ�ͷ�ļ�
@version 2005-12-12 shockingli
*/

#pragma once
#include <string>
#include <time.h>

#include "Winbase.h"
#include "Winuser.h"
#include <vector>
#include <set>
#include <string>
#include <deque>
#include <cstring>
using namespace std;

extern TCHAR LOGFILTER_ERROR[];
extern TCHAR LOGFILTER_INFO[];
extern int g_nLogLevel;
#define LOGL_Error 1
#define LOGL_Info 2
#define LOGL_Dev 3

#define DIV_K 1024

#ifdef __cplusplus
#define extc extern "C"
#else
#define extc
#endif
#define  PERF_ALERT_TIME 200

extc void * _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)

///base: log base,����ָ����ࡣ
/**
@ingroup ov_TXLog
*/
typedef struct tagLogObjBase
{
	UINT  uSize;
	INT    nLogLevel;
	INT    nLineNumber;
	DWORD  dwProcessId;
	DWORD  dwThreadId;
	DWORD  dwTick;
	DWORD  dwExtraDataLen;
	time_t tmTime;
	INT64  perfc;
	//w: source filename
	//w: function name
	//w: filter
	//w: text
	//module name:fixed 16 bytes
}tagLogObjBase;

typedef struct tagLogObj
{
	tagLogObjBase base;
	PVOID  pExtraData;
	LPCWSTR pszSourceFileName;
	LPCWSTR pszFuncName;
	PVOID  pRetAddr;
}tagLogObj;

// ��ΪC-style����
//EXTERN_C 
// ��Ϊ�ⲿ�ŶԵײ��ʹ�÷������⣬�˴����˸Ķ������ǸĻ�C++���÷�ʽ
void TXLog_DoTXLogVW(tagLogObj* pLogObj, LPCWSTR pszFilter, LPCWSTR pszFmt, va_list vg);


void TXLog_DoLogV(LPCWSTR pszSourceFileName,
						   INT nLineNumber, LPCWSTR pszFuncName, int nLogLevel,LPCWSTR pszFilter, LPCWSTR pszFmt, va_list vg);

#ifdef __cplusplus
__inline void TXLog_DoLog(LPCWSTR pszSourceFileName, INT nLineNumber, LPCWSTR pszFuncName, int nLogLevel,
						  LPCWSTR pszFilter, LPCWSTR pszCon)
{
	return TXLog_DoLogV(pszSourceFileName, nLineNumber, pszFuncName, nLogLevel, pszFilter, _T("%s"), (va_list)&pszCon);
}
template <class T>
__inline void TXLog_DoLog(LPCWSTR pszSourceFileName, INT nLineNumber, LPCWSTR pszFuncName, int nLogLevel,
						  LPCWSTR pszFilter, __in_z __format_string LPCWSTR pszCon, T obj, ...)
{
	va_list va = (va_list)& reinterpret_cast<const volatile char&>(obj);
	return TXLog_DoLogV(pszSourceFileName, nLineNumber, pszFuncName, nLogLevel, pszFilter, pszCon, va);
}

__inline void TXLog_DoLog(LPCWSTR pszSourceFileName,INT nLineNumber, LPCWSTR pszFuncName, int nLogLevel,
						  DWORD dwExtLen, const void * pExtData, LPCWSTR pszFilter, __in_z __format_string LPCWSTR pszFmt, ...);

namespace TXLog
{
	/// ��������е�log
	void FlushLog();
	/// ���log, ͬʱ�ù��������������ڴ�й©���������ڷ���crash������ڴ��е�logʱʹ�ã�ƽʱ����ʹ�á�
	void FlushLogLeakMemory();
	/// �����Ƿ�д���ļ�,��ҪLog����ʱ,��ֹд�뵽�ļ�.
	void SetWriteFileOpt(BOOL bWrite);
	/// ȡsessionֵ����bugreport�á�
	DWORD GetSession();
	/// ȡlog
	std::string GetLogByFilter(const CString & strLogPath, DWORD dwSession, const CString & strFilter, DWORD dwMaxItem);
	/// ��ȡ��ǰLogger�Ĵ��ھ��
	HWND GetLoggerWnd();
};
#else
__inline void TXLog_DoLog(LPCWSTR pszSourceFileName, INT nLineNumber, LPCWSTR pszFuncName, int nLogLevel,
						  LPCWSTR pszFilter, LPCWSTR pszFmt, ...)
{
	va_list vg = (va_list)(&pszFmt+1);
	return TXLog_DoLogV(pszSourceFileName, nLineNumber, pszFuncName, nLogLevel, pszFilter, pszFmt, vg);
}
#endif


#ifdef _DEBUG
extern BOOL g_bAssert;
#define WYASSERT(exp)  do { if(g_bAssert && !(exp)){  _asm { int 3 } } } while (0)
#else 
#define WYASSERT(exp)
#endif

#define LogError(fmt,...) do { TXLog_DoLog(__FILEW__, __LINE__, __FUNCTIONW__, LOGL_Error, LOGFILTER_ERROR, fmt, __VA_ARGS__);WYASSERT(FALSE);}while(0)
#define LogErrorEx(flt,fmt,...) TXLog_DoLog(__FILEW__, __LINE__, __FUNCTIONW__, LOGL_Error, flt,fmt, __VA_ARGS__)
#define LogFinal(flt,fmt,...) TXLog_DoLog(__FILEW__, __LINE__, __FUNCTIONW__, LOGL_Info, flt,fmt, __VA_ARGS__)
#define LogDev(flt,fmt,...)	  TXLog_DoLog(__FILEW__, __LINE__, __FUNCTIONW__, LOGL_Dev, flt,fmt, __VA_ARGS__)


/// �������ݴ����������.
extc PVOID GetDataProcessFillterAddress(LPCSTR pFuncNameOrEventName);

/// �������ݴ��������
/**
@ingroup ov_TXLog
*/
#define DATA_PROCESS_FILTER_HELPER(pFuncNameOrEventName, ...) do	{ \
	PVOID pfunc = GetDataProcessFillterAddress(pFuncNameOrEventName); \
	if(pfunc) \
	(* reinterpret_cast<void (__cdecl*) (...)>(pfunc))(__VA_ARGS__);\
}while(0)

