
// HTTPLib.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CHTTPLibApp: 
// �йش����ʵ�֣������ HTTPLib.cpp
//

class CHTTPLibApp : public CWinApp
{
public:
	CHTTPLibApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CHTTPLibApp theApp;