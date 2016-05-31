/** @file 
*   @brief   TXLog 实现文件
*   @version 2005-12-12 shockingli
*/

#include "stdafx.h"
#define TX_DISABLE_EXAMDATA
#include "Log.h"
#include "LogPrivate.h"
#include "atltime.h"
#include "UtilCoreConvert.h"
#include "TXPerfLogTime.h"
#include <ShlObj.h>

#define WRITEFILE_TIMEOUT	100

////单次登录日志文件最大的size，从50M变成100M，当部分人开启log3的时候，能记录更长时间的信息
#define ONCELUANCH_LOGFILE_MAX_SIZE		100*1024*1024

////一般情况下，日志文件最大size（启动QQ就检查），从5M变成10M，这样3个log文件可以保存更多的登录次数的日志信息
#define COMMON_LOGFILE_MAX_SIZE			10*1024*1024

//#pragma comment (linker, "/include:_g_ac_ptr")

//zjkw 2006-09-18 for perf of memusage of im...
//#include "psapi.h"
//#pragma comment (lib, "Psapi.lib")

TCHAR LOGFILTER_ERROR[]= _T("WY_ERROR");
TCHAR LOGFILTER_INFO[]= _T("WY_INFO");
int  g_nLogLevel = LOGL_Info;

#ifdef _DEBUG
BOOL g_bAssert = FALSE;
void InitAssertFlag()
{
	TCHAR szVar[256];
	DWORD dwRet = ::GetEnvironmentVariable(_T("WYASSERT"), szVar, _countof(szVar));
	if (dwRet != 0)
	{
		CString strVal = szVar;
		if (strVal == _T("TRUE"))
		{
			g_bAssert = FALSE;
		}
	}
}

#else
inline static void MainThreadCheck()
{
}

inline void InitAssertFlag()
{

}
#endif
//log.cpp和logviewerdlg.cpp中都有要同步
TCHAR chLogKeys[64] = 
{
	0x29e9, 0x76ce, 0x45e1, 0x7e11, 0x5133, 0x60e9, 0x61e3, 0x2c54,
	0x0bda, 0x7bec, 0x5e37, 0x4481, 0x3c12, 0x2048, 0x7458, 0x11be,
	0x59b8, 0x1a75, 0x62b1, 0x2d59, 0x5b39, 0x0040, 0x3ca5, 0x1eb7,
	0x25b0, 0x7067, 0x7bd2, 0x378e, 0x484e, 0x7063, 0x3697, 0x0054,
	0x06ff, 0x5c85, 0x1fd3, 0x48f8, 0x784a, 0x1e44, 0x33f0, 0x31cf,
	0x3a79, 0x1cd6, 0x3721, 0x40d1, 0x03fb, 0x1bfd, 0x68e7, 0x5705,
	0x1306, 0x7d24, 0x10da, 0x1a3a, 0x4081, 0x16aa, 0x48a6, 0x26f8,
	0x3a20, 0x157e, 0x2754, 0x1eb6, 0x27d1, 0x1cec, 0x31cb, 0x664d,
};

HMODULE g_hCurModule = NULL;
namespace 
{
	class CTXLoger
	{
		class CLogFile
		{			
		public:
			CLogFile() : m_hFile(INVALID_HANDLE_VALUE) {}			
			~CLogFile(){ if ( m_hFile != INVALID_HANDLE_VALUE )	CloseHandle(m_hFile); }
			BOOL Open(TCHAR * pchFilePath)
			{ 
				m_hFile = CreateFile(pchFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); return IsReady(); 
			}
            BOOL Close()
            {
                if (m_hFile != INVALID_HANDLE_VALUE)
                {
                    return CloseHandle(m_hFile);
                }
                return FALSE;
            }
            BOOL IsReady() { return (m_hFile != INVALID_HANDLE_VALUE); }
			void SeekEnd() { if ( IsReady() ) SetFilePointer(m_hFile, 0, 0, FILE_END); }
			DWORD Write(void *pBuf, UINT uSize) 
			{ 
				DWORD dwWritten = 0; 
				if ( m_hFile != INVALID_HANDLE_VALUE ) 
				{
					WriteFile(m_hFile, pBuf, uSize, &dwWritten, 0);
				}
				return dwWritten; 
			}
            DWORD GetSize()
            {
				if (m_hFile != INVALID_HANDLE_VALUE) 
				{
					return GetFileSize(m_hFile, NULL);
				}
                return (DWORD)-1;
            }
		protected:
			HANDLE m_hFile;
		};
	
	protected:	
		CWYLock m_Lock;
		CWYLock m_LockSave;		

	public:
		static CTXLoger & GetLoger();
		HWND  m_hWnd;
		static void DestroyLogerWindow()
		{
			if (GetLoger().m_hWnd)
			{
				DestroyWindow(GetLoger().m_hWnd);
				GetLoger().m_hWnd = NULL;
			}
		}

	public:		
		DWORD m_dwPid;        //process id.
		DWORD m_dwCookie;     //session cookie
		DWORD m_dwStTime;     //process startup time

		TCHAR m_szLogFileName[MAX_PATH];	
		HWND  m_hViewer;
		DWORD m_dwMaxQueueLogs;
		bool  m_bLorder;
		bool  m_bBuffer;
		bool  m_bWriteOpt;
		bool  m_bDisable;
	public:
		typedef struct {
			enum{SIZE=32*1024-sizeof(size_t)-sizeof(int)};
			size_t size;
			int ref; // 同时有几个线程正在Add Log，Flush时必须要ref为0
			BYTE data[SIZE];
		}LOGBUFFER;
		LOGBUFFER* m_pCurBuffer;
		typedef std::vector<LOGBUFFER*> VECTOR_LOGBUFFER;
		VECTOR_LOGBUFFER m_saveList;
		std::set<CWYString> m_setFilters;
	private:
		static CTXLoger* s_pLoger;
	private:
		void RenameFileList(DWORD dwSize)
		{

			WIN32_FIND_DATA wfd;
			HANDLE hFind = FindFirstFile(m_szLogFileName, &wfd);
			if(hFind != INVALID_HANDLE_VALUE)
			{
				if(wfd.nFileSizeHigh || wfd.nFileSizeLow > dwSize)
				{
					TCHAR fn1[MAX_PATH];
					TCHAR fn2[MAX_PATH];
					TCHAR fn3[MAX_PATH];

					wcsncpy_s(fn1, m_szLogFileName, _countof(fn1));
					wcsncpy_s(fn2, m_szLogFileName, _countof(fn2));
					wcsncpy_s(fn3, m_szLogFileName, _countof(fn3));

					wcsncat_s(fn1, L".1", _countof(fn1));
					wcsncat_s(fn2, L".2", _countof(fn2));
					wcsncat_s(fn3, L".3", _countof(fn3));
					for (int i=0; i<3; ++i)
					{
						if (DeleteFile(fn3)) break;
						if (i<2) Sleep(40);
					}
					MoveFile(fn2, fn3);
					MoveFile(fn1, fn2);
					MoveFile(m_szLogFileName, fn1);
					DeleteFile(m_szLogFileName);
				}
				FindClose(hFind);
			}
		}
	public:

		VOID Init()
		{

			m_dwPid = GetCurrentProcessId();

			m_dwCookie = GetSessionCookie();
			m_dwStTime = GetStartupTime();

			{
				TCHAR fn[MAX_PATH] = {0};
				GetModuleFileName(NULL, fn, MAX_PATH);
				TCHAR * pch = fn + wcslen(fn);
				while (pch > fn && !(pch[-1] == '\\' || pch[-1] == '/')) --pch;
				memmove(fn, pch, (wcslen(pch)+1) * sizeof(TCHAR));
				pch = wcsrchr(fn, '.');
				if (pch) *pch = 0;
				if (fn[0] == 0)
				{
					wcsncpy_s(fn, L"log.tlg", _countof(fn));
				}
				else
				{
					wcsncat_s(fn, L".tlg", _countof(fn));
				}
				m_szLogFileName[0] = 0;
				SHGetSpecialFolderPath(::GetDesktopWindow(), m_szLogFileName, CSIDL_APPDATA, TRUE);
				if (m_szLogFileName[0] && m_szLogFileName[wcslen(m_szLogFileName)-1] != '\\')
				{
					wcsncat(m_szLogFileName, L"\\", _countof(m_szLogFileName));
				}
				wcsncat(m_szLogFileName, L"Tencent\\", _countof(m_szLogFileName));
				CreateDirectory(m_szLogFileName, NULL);
				wcsncat(m_szLogFileName, L"Logs\\", _countof(m_szLogFileName));
				CreateDirectory(m_szLogFileName, NULL);
				TCHAR szWY[] =_T("WY_");
				wcsncat(m_szLogFileName, szWY, _countof(m_szLogFileName));
				wcsncat(m_szLogFileName, fn, _countof(m_szLogFileName));
			}

		
			if (GetProcAddress(GetModuleHandle(0), "NoTXLog") != NULL)
			{
				m_bDisable = true;
				return ;
			}
			RenameFileList(COMMON_LOGFILE_MAX_SIZE);
		}

		LRESULT OnSetLevelWithFilter(tagLogLevelParam* pParam)
		{
			if(pParam)
			{
				g_nLogLevel = static_cast<INT>(pParam->dwLogLevel);
				if(pParam->pszFilter)
				{
					CWYString strFilter = pParam->pszFilter;
					strFilter.Remove(_T(' '));
					strFilter += _T('|');

					int nPos = 0;
					m_setFilters.clear();
					CWYString strSubFilter = _T("");
					while((nPos = strFilter.Find(_T('|'))) != -1)
					{
						strSubFilter = strFilter.Left(nPos);
						strFilter.Delete(0, nPos+1);
						if(strSubFilter.IsEmpty())
						{
							continue;
						}
						if(m_setFilters.find(strSubFilter) == m_setFilters.end())
						{
							m_setFilters.insert(strSubFilter);
						}
					}
					delete [] pParam->pszFilter;
				}
			}
			return 1;
		}


		LRESULT ProcessCommandRequest(WPARAM wParam, LPARAM lParam)
		{
			switch(wParam)
			{
			case LOGGERMSG_WP_SETVIEWER:
				m_hViewer = (HWND)lParam;
				break;
			case LOGGERMSG_WP_SETLEVEL:
				g_nLogLevel = static_cast<INT>(lParam);
				break;
			case LOGGERMSG_WP_SETLEVEL_WITHFILTER:
				OnSetLevelWithFilter((tagLogLevelParam*)lParam);
				break;
			case LOGGERMSG_WP_GETLEVEL:
				return (LRESULT)g_nLogLevel;
			case LOGGERMSG_WP_GETVIEWER:
				return (LRESULT) m_hViewer;
			case LOGGERMSG_WP_SETBUFOPT:
				m_bBuffer = (lParam != 0);
				break;
			case LOGGERMSG_WP_GETBUFOPT:
				return m_bBuffer;
			default:
				break;
			}
			return 1;
		}

		static LRESULT CALLBACK LogerCommandWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch(uMsg)
			{
			case LOGGERMSG_CONTROL:
				return GetLoger().ProcessCommandRequest(wParam, lParam);
			default:
				break;
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}	
		
		HWND SetupWindow()
		{
			WNDCLASSEX wc    = {sizeof(wc)};
			wc.hInstance     = GetModuleHandle(0);
			wc.lpszClassName = LOG_CLS_NAME;
			wc.lpfnWndProc   = &LogerCommandWndProc;
			wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
			WNDCLASSEX wc2;
			if( GetClassInfoEx(wc.hInstance, wc.lpszClassName, &wc2) )
			{
				return NULL;
			}
			else
			{
				RegisterClassEx(&wc);
				m_hWnd = CreateWindowEx(0, wc.lpszClassName, _T("loger command window"), WS_OVERLAPPED, 0, 0, 100, 100, 
					HWND_MESSAGE, NULL, wc.hInstance, NULL);
				ShowWindow(m_hWnd, SW_HIDE);
				//bugfix http://tapd.oa.com/v3/10014001/bugtrace/bugs/view?bug_id=1010014001009315692
				//TODOUtil::Misc::AddToDeadQueue(Util::Misc::DEADQUEUE_PRIORITY_LAST0, &DestroyLogerWindow);
				return m_hWnd;
			}
		}


		CTXLoger()
		{
			InitAssertFlag();
			m_bBuffer   = 1;
			m_hViewer   = 0;
			m_hWnd      = 0;
			m_bLorder   = 1;
#ifndef VER_FINAL_RELEASE
			g_nLogLevel = LOGL_Dev;
#else
			g_nLogLevel = LOGL_Info;
#endif
			m_dwMaxQueueLogs = MAX_QUEUE_LOGS;
			m_bWriteOpt = TRUE;
			m_bDisable  = false;
			m_setFilters.clear();

			m_pCurBuffer = NULL;

			vector<CWYString> args;
			Util::Convert::SplitCmdString(GetCommandLine(), args);

			for (size_t i=1, usz=args.size(); i<usz; ++i)
			{
				const LPCTSTR strSwitch = args[i];
				if (strSwitch)
				{
					//根据lyy发映：这个log开关可能要给用户来设定，所以，用个简单点的命令行控制。
					if (strSwitch[0] == _T('/') && iswdigit(strSwitch[1]) && strSwitch[2] == 0)
					{
						g_nLogLevel = strSwitch[1] - '0';
					}
					else if (wcsncmp(strSwitch, _T("/loglevel:"), 10) == 0)
					{
						g_nLogLevel = _ttoi(strSwitch+10);
					}
					else if (wcsncmp(strSwitch, _T("/logbuf:"), 8) == 0)
					{
						Util::Convert::StringToDWordW(strSwitch, m_dwMaxQueueLogs); m_dwMaxQueueLogs += 8;//	m_dwMaxQueueLogs = _ttoi(strSwitch+8);//21UIN_find_risk++

						if (m_dwMaxQueueLogs <= 1)
						{
							m_bBuffer = false;
						}
					}
				}
			}
			if ( !SetupWindow() )
			{
				m_bLorder = 0;
			}
			Init();
		}

		~CTXLoger()
		{
			if (m_hWnd)
			{
				DestroyWindow(m_hWnd);
				m_hWnd = NULL;
			}

			Flush(true);
			g_nLogLevel = -10000;
		}
			
		DWORD GetSessionCookie()
		{

			LARGE_INTEGER li = {0};
			QueryPerformanceCounter(&li);
			DWORD dw = GetTickCount();
			dw ^= GetCurrentProcessId();
			dw ^= li.LowPart;
			dw ^= li.HighPart;
			dw ^= static_cast<DWORD>( time(0) );
			if (dw == 0) dw = 1; //must not be NULL, NULL for special case.
			return dw;
		}
		DWORD GetStartupTime()
		{
			FILETIME crtime,nowtime,ttime;
			ULARGE_INTEGER cr,now;

			GetProcessTimes(GetCurrentProcess(),&crtime,&ttime,&ttime,&ttime);
			GetSystemTimeAsFileTime(&nowtime);
			DWORD nowtk = GetTickCount();

			cr.LowPart  = crtime.dwLowDateTime;
			cr.HighPart = crtime.dwHighDateTime;
			now.LowPart = nowtime.dwLowDateTime;
			now.HighPart= nowtime.dwHighDateTime;
			ULONGLONG ut = now.QuadPart - cr.QuadPart;
			ut /= 10000;
			DWORD dwThreadStartTime = nowtk - (DWORD)ut;
			return dwThreadStartTime;
		}	

		BOOL LockedWriteLog(bool bFree)
		{

			if (!m_bDisable)
			{
				CLogFile LogFile;	
				
				if (!LogFile.Open(m_szLogFileName))
				{
					//加个保护，如果文件只读则去掉只读
					BOOL bNeedTry = TRUE;
					DWORD dwAttributes = GetFileAttributes(m_szLogFileName);
					if (dwAttributes & FILE_ATTRIBUTE_READONLY)
					{
						SetFileAttributes(m_szLogFileName, dwAttributes & (~(ULONG)FILE_ATTRIBUTE_READONLY));
						if (LogFile.Open(m_szLogFileName))
						{
							bNeedTry = FALSE;
						}							
					}

					if(bNeedTry)
					{
						UINT i = 0;
						for (; i < 3; i++)		 
						{
							Sleep(30);
							if (LogFile.Open(m_szLogFileName))
								break;
						}
						if (i == 3)
							return FALSE;
					}					
				}
#ifdef VER_FINAL_RELEASE
                if (LogFile.GetSize() > ONCELUANCH_LOGFILE_MAX_SIZE)
                {
                    if (LogFile.Close())
                    {
                        RenameFileList(ONCELUANCH_LOGFILE_MAX_SIZE);
                        if (!LogFile.Open(m_szLogFileName))
                        {
                            return FALSE;
                        }
                    }
                    else
                    {
                        return FALSE;
                    }
                }
#endif
				LogFile.SeekEnd();
				VECTOR_LOGBUFFER::iterator it = m_saveList.begin();
				for (it; it != m_saveList.end(); ++it)
				{
					LOGBUFFER* pSaveBuffer = (*it);
					while(pSaveBuffer->ref) Sleep(1);
					LogFile.Write(pSaveBuffer->data, pSaveBuffer->size);
					// crash 时不delete日志能记录在dump里
					if (bFree)
					{
						delete pSaveBuffer;
					}
				}
				if (bFree)
				{
					m_saveList.clear();
				}
			}
			return TRUE;
		}

		VOID AddLog(tagLogObjExt * pLog)
		{
			pLog->dwProcessId = m_dwPid;
			pLog->dwCookie = m_dwCookie;
			pLog->dwStTime = m_dwStTime;
			if (m_hViewer && m_hWnd)
			{
				if( IsWindow(m_hViewer) )
				{
					COPYDATASTRUCT cd;
					cd.dwData = LOGGER_COPYDATA_DWDATA_VIEWLOG;
					cd.cbData = pLog->uSize;
					cd.lpData = pLog;
					SendMessage(m_hViewer, WM_COPYDATA, (WPARAM)m_hWnd, (LPARAM)&cd);
				}
				else
				{
					m_hViewer = 0;
				}
			}
		}
		VOID Flush(bool bFree)
		{
			m_Lock.Lock();
			if (m_pCurBuffer && m_pCurBuffer->size != 0)
			{
				m_LockSave.Lock();
				m_saveList.push_back(m_pCurBuffer);
				m_LockSave.Unlock();
				m_pCurBuffer = NULL;
			}
			m_Lock.Unlock();
			FlushSaveList(bFree);
		}
		VOID FlushSaveList(bool bFree)
		{
			m_LockSave.Lock();
			if (m_saveList.size() > 0)
			{
				LockedWriteLog(bFree);
			}
			m_LockSave.Unlock();
		}
		static DWORD WINAPI _WorkWriteLogThread(LPVOID param)
		{
			CTXLoger* txLoger = (CTXLoger*)param;
			txLoger->FlushSaveList(true);
			return 0;
		}

		void InternalLog(tagLogObj* LogObj, LPCWSTR pwszFilter, LPCWSTR pwszText)
		{
			while( iswspace( *pwszText) ) ++ pwszText;
			while( iswspace( *pwszFilter) ) ++ pwszFilter;
			UINT uLenFilter = wcslen(pwszFilter);
			UINT uLenText = wcslen(pwszText);
			while(uLenFilter>0 && iswspace(pwszFilter[uLenFilter-1]) ) -- uLenFilter;
			while(uLenText>0 && iswspace(pwszText[uLenText-1]) ) -- uLenText;
			UINT uLenFunc = wcslen( LogObj->pszFuncName );
			UINT uLenAll = (uLenFilter + uLenText + uLenFunc + 3) * sizeof(WCHAR) + LogObj->base.dwExtraDataLen;
			uLenAll = (uLenAll + 3) & ~3;

			void*  pData = NULL;
			LOGBUFFER* pBuffer = NULL;
			size_t size = sizeof(tagLogObjExt) + uLenAll;
			BOOL bFlushLog = FALSE;

			m_Lock.Lock();
			if (m_pCurBuffer)
			{
				static ULONGLONG s_uLastFlushTime = ::Util::Time::TXGetTickCount();
				ULONGLONG uCur = ::Util::Time::TXGetTickCount();

				if (LogObj->base.nLogLevel==1 && (uCur - s_uLastFlushTime)> 1000 * 2 && m_pCurBuffer->size > 0)
				{
					bFlushLog = TRUE;

				}
				if (m_pCurBuffer->size + size >= LOGBUFFER::SIZE ||bFlushLog)
				{
					m_LockSave.Lock();
					m_saveList.push_back(m_pCurBuffer);
					m_LockSave.Unlock();
					m_pCurBuffer = NULL;
					bFlushLog = TRUE;
					s_uLastFlushTime = uCur;
				}
			}
			if (m_pCurBuffer == NULL)
			{
				m_pCurBuffer = (LOGBUFFER*)malloc(sizeof(LOGBUFFER));
				if (m_pCurBuffer)
				{
					m_pCurBuffer->size = 0;
					m_pCurBuffer->ref = 0;
				}
			}
			if (m_pCurBuffer)
			{
				if (m_pCurBuffer->size + size < LOGBUFFER::SIZE)
				{
					pBuffer = m_pCurBuffer;
					pData = pBuffer->data + pBuffer->size;
					pBuffer->size += size;
					InterlockedIncrement((volatile LONG*)&pBuffer->ref);
				}
			}
			m_Lock.Unlock();

			// DLLMain函数LogFinal会发生死锁，所以移到Lock外
			if (bFlushLog && m_bWriteOpt) 
			{
				QueueUserWorkItem(_WorkWriteLogThread, (PVOID)this, WT_EXECUTEDEFAULT);
			}

			if (pData != NULL)
			{
				tagLogObjExt * pExt = (tagLogObjExt*)pData;

				LPWSTR pszStr = (LPWSTR) ( (PBYTE)pExt + sizeof(tagLogObjExt) );
				memcpy(pExt, LogObj, sizeof(tagLogObjBase));
				pExt->uSize = static_cast<DWORD>( sizeof(tagLogObjExt) + uLenAll );

				pExt->wFuncName = static_cast<WORD>( (PBYTE)pszStr - (PBYTE)pExt );
				memcpy( pszStr, LogObj->pszFuncName, (uLenFunc+1)*sizeof(WCHAR) );
				pszStr += uLenFunc + 1;

				pExt->wFilter = static_cast<WORD>( (PBYTE)pszStr - (PBYTE)pExt );
				memcpy( pszStr, pwszFilter, (uLenFilter)*sizeof(WCHAR) );
				pszStr[uLenFilter] = 0^LOGKEY(uLenFilter);
				for (UINT i=0; i<uLenFilter; ++i)
				{
					pszStr[i] ^= LOGKEY(i);
				}
				pszStr += uLenFilter+1;
				//*pszStr++ = 0;

				pExt->wText = static_cast<WORD>( (PBYTE)pszStr - (PBYTE)pExt );
				memcpy( pszStr, pwszText, (uLenText)*sizeof(WCHAR) );
				pszStr[uLenText] = 0^LOGKEY(uLenText);
				for(UINT i=0; i<uLenText; ++i)
				{
					pszStr[i] ^= LOGKEY(i);
				}
				pszStr += uLenText+1;
				//*pszStr++ = 0;

				pExt->wExt = static_cast<WORD>( (PBYTE)pszStr - (PBYTE)pExt );
				memcpy( pszStr, LogObj->pExtraData, LogObj->base.dwExtraDataLen);

				MEMORY_BASIC_INFORMATION mb;
				WCHAR chMod[MAX_PATH];
				chMod[0] = 0;
				if (LogObj->base.nLogLevel < 4 && VirtualQuery(LogObj->pRetAddr, &mb, sizeof(mb))/* &&
					GetModuleFileName( (HMODULE)mb.AllocationBase, chMod, sizeof(chMod)/sizeof(WCHAR))*/)
				{
					//怀疑GetModuleFileName卡，暂时去掉
					_sntprintf(pExt->chModuleName, MOD_NAME_LEN, _T("0x%p"), mb.AllocationBase);
					pExt->chModuleName[MOD_NAME_LEN-1] = 0;
					/*
					LPWSTR pch = wcsrchr(chMod, L'\\');
					if(pch) ++ pch;
					else pch = chMod;
					wcsncpy(pExt->chModuleName, pch, MOD_NAME_LEN);
					pExt->chModuleName[MOD_NAME_LEN-1] = 0;
					*/
				}
				else
				{
					pExt->chModuleName[0] = 0;
				}
				LPCWSTR pSrc1 = wcsrchr(LogObj->pszSourceFileName,_T('\\'));
				LPCWSTR pSrc2 = wcsrchr(LogObj->pszSourceFileName,_T('/'));
				pSrc1 = max(pSrc1,pSrc2);
				if (!pSrc1) pSrc1 = LogObj->pszSourceFileName;
				else ++ pSrc1;
				wcsncpy(pExt->chSrcName, pSrc1, SRC_NAME_LEN);
				pExt->chSrcName[SRC_NAME_LEN-1] = 0;
				AddLog(pExt);
				InterlockedDecrement((volatile LONG*)&pBuffer->ref);
				if (m_bBuffer == 0)
				{
					Flush(true);
				}
			}

		}
	};

	void InternalLog(tagLogObj* LogObj, LPCWSTR pwszFilter, LPCWSTR pwszText)
	{
		CTXLoger::GetLoger().InternalLog(LogObj, pwszFilter, pwszText);
	}

	CTXLoger * g_pLoger = NULL;
	static void DestroyLoger()
	{
		delete g_pLoger;
		g_pLoger = NULL;
	}
	static void InitLoger()
	{
		atexit(DestroyLoger);
		g_pLoger = new CTXLoger;
	}
	CTXLoger & CTXLoger::GetLoger()
	{
		if (NULL == g_pLoger)
		{
			InitLoger();
		}
		return *g_pLoger;
	}
}
extern "C" void * e_txweakobj_6 = InitLoger;

PVOID GetDataProcessFillterAddress(LPCSTR pFuncNameOrEventName)
{
	//注意：这一行是让logger创建，不要注释掉
	CTXLoger::GetLoger();
	return 0;
}

namespace TXLog
{
	void FlushLog()
	{
		CTXLoger::GetLoger().Flush(true);
	}

	void FlushLogLeakMemory()
	{
		// 发生了crash，内存靠不住了，普通的操作也要加上保护。
		__try
		{
			CTXLoger::GetLoger().Flush(false);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			;
		}
	}

	void SetWriteFileOpt(BOOL bWrite)
	{
		FlushLog();
		CTXLoger::GetLoger().m_bWriteOpt = bWrite != 0;
	}

	DWORD GetSession()
	{
		CTXLoger & lg = CTXLoger::GetLoger();
		return lg.m_dwCookie;
	}
}


void TXLog_DoTXLogVW(tagLogObj* pLogObj, LPCWSTR pszFilter, LPCWSTR pszFmt, va_list vg)
{
	if (pLogObj==NULL || pszFilter==NULL || pszFmt==NULL || vg==NULL)
	{
		return;
	}

	if (CTXLoger::GetLoger().m_bDisable)
	{
		return ;
	}

	pLogObj->pRetAddr = _ReturnAddress();
	
	int nLogLevel = g_nLogLevel;

	if( nLogLevel < 0 || pLogObj->base.nLogLevel > nLogLevel ) return;
	

#ifdef VER_FINAL_RELESE
    pLogObj->pszSourceFileName = L"file";
    pLogObj->pszFuncName = L"func";
#endif

	static int once = 0;
	if (once == 0)
	{
		once = 1;
		//XXX_API
		LARGE_INTEGER lpFrequency;
		QueryPerformanceFrequency(&lpFrequency);
		LogFinal(L"PerfBenchmark",_T("%I64d"),lpFrequency.QuadPart);
	}

	CWYString str;
	str.FormatV(pszFmt, vg);
	if(str.GetLength() >= 200 *1024 )
	{
		return ;
	}
	return InternalLog(pLogObj, pszFilter, str);
}



string TXLog::GetLogByFilter(const CWYString & strLogPath, DWORD dwSession, const CWYString & strFilter, DWORD dwMaxItem)
{
	string sr;
	if (dwSession == 0 || dwMaxItem == 0)
	{
		//no log.
		return sr;
	}
	HANDLE hf = CreateFile(strLogPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if (hf == INVALID_HANDLE_VALUE) return sr;
	tagLogObjExt * pExt = 0;
	DWORD dwExtLen = 0;
	std::deque<string> veclog;
	for (;;)
	{
		DWORD dwLen, dwRead;
		bool bItemOk = false;
		if (ReadFile(hf, &dwLen, sizeof(dwLen), &dwRead, NULL) && dwRead == 4 && dwLen < 0x10000 && dwLen>4)
		{
			if (dwExtLen < dwLen)
			{
				dwExtLen = dwLen;
				free(pExt);
				pExt = (tagLogObjExt*) malloc(dwExtLen);
			}
			pExt->uSize = dwLen;
			if (ReadFile(hf, &pExt->nLogLevel, dwLen-4, &dwRead, NULL) && dwRead == dwLen-4)
			{
				if (pExt->wFuncName < dwLen && pExt->wFilter < dwLen && pExt->wText < dwLen && pExt->wExt <= dwLen)
				{
					bItemOk = true;
					if (pExt->dwCookie == dwSession)
					{
						LPTSTR filter = (LPTSTR)((PBYTE)pExt + pExt->wFilter);
						std::wstring strItemFilter;
						strItemFilter.reserve(256);
						for (UINT i=0; ; ++i)
						{
							TCHAR ch = filter[i] ^ LOGKEY(i);
							if (ch) strItemFilter += ch;
							else break;
						}
						if (strFilter.GetLength() == 0 || _tcsstr(strItemFilter.c_str(), strFilter) > 0)
						{
							//ok, match the item,add it.
							if (veclog.size() >= dwMaxItem)
							{
								veclog.pop_front();
							}
							veclog.push_back(string((char*)pExt, dwLen));
						}
					}
				}
			}
		}
		if (!bItemOk)
		{
			break;
		}
	}
	free(pExt);
	CloseHandle(hf);
	for (std::deque<string>::iterator pIter = veclog.begin(); pIter!=veclog.end(); ++pIter)
	{
		sr += * pIter;
	}
	return sr;
}

HWND TXLog::GetLoggerWnd()
{
	return CTXLoger::GetLoger().m_hWnd;
}

void TXLog_DoLogV(LPCWSTR pszSourceFileName,
	INT nLineNumber, LPCWSTR pszFuncName, int nLogLevel,LPCWSTR pszFilter, LPCWSTR pszFmt, va_list vg)
{
	tagLogObj logobj;
	logobj.pszSourceFileName = pszSourceFileName;
	logobj.pszFuncName = pszFuncName;
	logobj.pExtraData = 0;
	logobj.pRetAddr = 0;

	logobj.base.nLineNumber = nLineNumber;
	logobj.base.dwProcessId = 0;
	logobj.base.dwThreadId = GetCurrentThreadId();
	logobj.base.nLogLevel = nLogLevel;
	logobj.base.dwTick = GetTickCount();
	logobj.base.dwExtraDataLen = 0;
	time(&logobj.base.tmTime);
	if (nLogLevel >= 4)
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		logobj.base.perfc = li.QuadPart;
	}
	else
	{
		logobj.base.perfc = 0;
	}
	TXLog_DoTXLogVW(&logobj,pszFilter,pszFmt,vg);
}

void TXLog_DoLog(LPCWSTR pszSourceFileName,INT nLineNumber, LPCWSTR pszFuncName, int nLogLevel,
	DWORD dwExtLen, const void * pExtData, LPCWSTR pszFilter, __in_z __format_string LPCWSTR pszFmt, ...)
{
	tagLogObj logobj;
	va_list vg;
	va_start(vg,pszFmt);

	logobj.pszSourceFileName = pszSourceFileName;
	logobj.pszFuncName = pszFuncName;
	logobj.pExtraData = (PVOID)pExtData;
	logobj.pRetAddr = 0;

	logobj.base.nLineNumber = nLineNumber;
	logobj.base.dwProcessId = 0;
	logobj.base.dwThreadId = GetCurrentThreadId();
	logobj.base.nLogLevel = nLogLevel;
	logobj.base.dwTick = GetTickCount();
	logobj.base.dwExtraDataLen = dwExtLen;
	TXLog_DoTXLogVW(&logobj,pszFilter,pszFmt,vg);
}