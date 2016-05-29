#include "StdAfx.h"
#include "EventNotifyMgr.h"
#include <WinBase.h>


static TCHAR const s_szClassName[] = _T("Lanx.EventNotifyMgr");
static UINT const  WM_EVENT_NOTIFY = WM_USER + 1000;
DWORD EventNotifyMgr::s_dwNotifyThreadID = 0;

DWORD EventNotifyMgr::RegisterClass()
{
	WNDCLASSEX wcex = {0};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= NULL;
	wcex.hIcon			= NULL;
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= s_szClassName;
	wcex.hIconSm		= NULL;

	return ::RegisterClassEx(&wcex);
}

LRESULT EventNotifyMgr::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	EventNotifyMgr * pThis =(EventNotifyMgr *)::GetWindowLongPtr(hWnd,GWLP_USERDATA);
	if (WM_CREATE == message)
	{
		LPCREATESTRUCT  pData = (LPCREATESTRUCT) lParam;
		::SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pData->lpCreateParams);
	}
	if(pThis == NULL)
	{
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}
	return pThis->HandleWndProc(hWnd,message,wParam,lParam);
}

LRESULT EventNotifyMgr::HandleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{	
	switch(message)
	{
	case WM_EVENT_NOTIFY:
		{
			MainThreadCheck();
			CTask * pTask = reinterpret_cast<CTask*> (lParam);
			pTask->Run();
			delete pTask;
		}
		break;
	}
	return ::DefWindowProc(hWnd, message, wParam, lParam);;
}


EventNotifyMgr::EventNotifyMgr()
	:m_hWnd(NULL)
{
	s_dwNotifyThreadID = ::GetCurrentThreadId();
	RegisterClass();
	CString strName;
	strName.Format(_T("%s_%u"),s_szClassName, (unsigned int)s_dwNotifyThreadID);
	m_hWnd = ::CreateWindowEx(0,s_szClassName,strName, WS_OVERLAPPEDWINDOW,CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE,NULL, ::GetModuleHandle(NULL),this);
}

EventNotifyMgr::~EventNotifyMgr()
{
	if (m_hWnd != nullptr)
	{
		::DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
}

BOOL EventNotifyMgr::PostEvent(CTask * pTask)
{
	return ::PostMessage(m_hWnd,WM_EVENT_NOTIFY,NULL,(LPARAM)pTask);
}

BOOL EventNotifyMgr::UnInitialize()
{
	if (m_hWnd != nullptr)
	{
		::DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
	return TRUE;
}