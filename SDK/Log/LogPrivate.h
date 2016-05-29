#pragma  once

#include "Log.h"

/// 最长支持显示的模块名长度
#define MOD_NAME_LEN   20
/// 最长支持显示的源文件名长度
#define SRC_NAME_LEN   32

#ifdef _DEBUG
/// 最多缓存的log条数
	#define MAX_QUEUE_LOGS 1000
#else
	#define MAX_QUEUE_LOGS 1000
#endif

/// logger命令窗的window class name
#ifdef NOT_USE_IM	
	#define LOG_CLS_NAME   _T("TXLOGGER_BYCORETEAM_NOTUSEIM")
#else 
	#ifdef SSOPLATFORM
		#define LOG_CLS_NAME   _T("TXLOGGER_BYCORETEAM_SSO")
	#else 
		#define LOG_CLS_NAME   _T("TXLOGGER_BYCORETEAM_WY")
	#endif
#endif

/// 外界与logger交互的消息号
#define LOGGERMSG_CONTROL WM_USER+1

/// 设置实时查看器的WPARAM号
#define LOGGERMSG_WP_SETVIEWER 0x19820820
/// 获取实时查看器的hwnd
#define LOGGERMSG_WP_GETVIEWER  (LOGGERMSG_WP_SETVIEWER+1)
/// 设置log级别的WPARAM号
#define LOGGERMSG_WP_SETLEVEL   (LOGGERMSG_WP_SETVIEWER+2)
/// 获取log级别的WPARAM号
#define LOGGERMSG_WP_GETLEVEL   (LOGGERMSG_WP_SETVIEWER+3)
/// 设置buf是否启用的wp.
#define LOGGERMSG_WP_SETBUFOPT  (LOGGERMSG_WP_SETVIEWER+4)
/// 获取buf是否启用的wp.
#define LOGGERMSG_WP_GETBUFOPT  (LOGGERMSG_WP_SETVIEWER+5)
/// 针对某些filter设置log级别
#define LOGGERMSG_WP_SETLEVEL_WITHFILTER	(LOGGERMSG_WP_SETVIEWER+6)

/// logger发到实时查看器的wm_copy_data消息，dwData表示发的data类型
#define LOGGER_COPYDATA_DWDATA_VIEWLOG  1
//#define LOGGER_COPYDATA_DWDATA_GETINFO  2

struct tagLogObjExt : tagLogObjBase
{
	DWORD dwCookie;
	DWORD dwStTime;
	WORD wFuncName;
	WORD wFilter;
	WORD wText;
	WORD wExt;
	WCHAR chModuleName[ MOD_NAME_LEN ];
	WCHAR chSrcName[SRC_NAME_LEN];
};

struct tagLogLevelParam
{
	DWORD dwLogLevel;
	TCHAR* pszFilter;
};

extern TCHAR chLogKeys[64];
#define LOGKEY(i) (chLogKeys[((unsigned long)i) % 64])
