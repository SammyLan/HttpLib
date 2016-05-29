#pragma once
#include <map>
#include <instance.hpp>
#include "Task.h"
#include <cassert>
using namespace std;

class EventNotifyMgr:public WYUtil::Instance<EventNotifyMgr>
{
	typedef EventNotifyMgr this_class;
public:
	EventNotifyMgr();
	~EventNotifyMgr();
	BOOL PostEvent(CTask * pTask);
	BOOL UnInitialize();
#ifdef MAIN_THREAD_CHECK
	inline static bool MainThreadCheck()
	{
		assert(s_dwNotifyThreadID == ::GetCurrentThreadId());
		if (s_dwNotifyThreadID != ::GetCurrentThreadId())
		{
			return false;
		}
		return true;
	}

#else
	inline static bool MainThreadCheck()
	{
		return true;
	}
#endif
protected:
	static DWORD RegisterClass();
private:
	static LRESULT  CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT HandleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	HWND			m_hWnd;
	static	DWORD	s_dwNotifyThreadID;
};
#define Main_Thread_Check()																					\
do																											\
{																											\
	if (!EventNotifyMgr::MainThreadCheck())																	\
	{																										\
		TXLog_DoLog(__FILEW__, __LINE__, __FUNCTIONW__, 1, _T("MainThreadCheck"), _T("非主线程调用接口"));	\
	}																										\
}																											\
while(0)