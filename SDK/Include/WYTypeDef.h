#pragma once
#include <atlcom.h>

typedef CComAutoCriticalSection		CWYLock;
typedef CComCritSecLock<CWYLock>	LockGuard;

typedef  CAtlStringA				CWYStringA;
typedef  CAtlStringW				CWYStringW;
typedef  CAtlString					CWYString;

#define SHAREED_PTR_DEF(className)	typedef std::shared_ptr<className> className##Ptr