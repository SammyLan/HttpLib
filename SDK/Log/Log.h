/** 
@file
@brief TXLog 实现的头文件
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

///base: log base,不含指针的类。
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

// 改为C-style导出
//EXTERN_C 
// 因为外部门对底层的使用方法问题，此处不宜改动，还是改回C++调用方式
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
	/// 输出缓存中的log
	void FlushLog();
	/// 输出log, 同时让故意让里面分配的内存泄漏掉，仅用于发生crash后，输出内存中的log时使用，平时不得使用。
	void FlushLogLeakMemory();
	/// 设置是否写入文件,当要Log性能时,禁止写入到文件.
	void SetWriteFileOpt(BOOL bWrite);
	/// 取session值，给bugreport用。
	DWORD GetSession();
	/// 取log
	std::string GetLogByFilter(const CString & strLogPath, DWORD dwSession, const CString & strFilter, DWORD dwMaxItem);
	/// 获取当前Logger的窗口句柄
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


/// 过程数据处理帮助函数.
extc PVOID GetDataProcessFillterAddress(LPCSTR pFuncNameOrEventName);

/// 过程数据处理帮助宏
/**
@ingroup ov_TXLog
*/
#define DATA_PROCESS_FILTER_HELPER(pFuncNameOrEventName, ...) do	{ \
	PVOID pfunc = GetDataProcessFillterAddress(pFuncNameOrEventName); \
	if(pfunc) \
	(* reinterpret_cast<void (__cdecl*) (...)>(pfunc))(__VA_ARGS__);\
}while(0)

